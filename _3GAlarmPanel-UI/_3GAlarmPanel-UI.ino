#include <cppQueue.h>
#include <MemoryFree.h>
#include <SimpleDHT.h>
#include <Adafruit_FONA.h>
#include <SoftwareSerial.h> 
#include <SerialCommand.h>
#include <EEPROM.h>

// PIN NUMBERS
// Digital
// Used :  0,1,2,3,4,5,6,7,8,9
// Free :  10,11,12,13
// Analog
// Used:   0,1
// Free:   2,3,4,5

// General Control
#define DEBUG true

// FONA Pins
#define FONA_TX 2
#define FONA_RX 3
#define FONA_RST 9
#define FONA_PWR 8

// DHT11 Pins
#define DHT11_DATA  5

// LED Pins
#define LED_GREEN  7
#define LED_RED    6
#define RED        B00000010
#define GREEN      B00000001
#define ORANGE     B00000011
#define OFF        B00000000

// Voltage Devider Pins
#define VOLTAGE_DATA0 A0 // This should be input voltage
#define VOLTAGE_DATA1 A1 // This should be battery voltage

// General Vars
#define EEPROMStart 513 // Start write and read for EEPROM
#define NUM_SAMPLES 10   // number of analog samples to take per voltage reading

// Serial Buffer
#define MAX_CMD_LEN 22

// Queue for SMS Messages
#define  MAX_MSG_LEN      56
#define  MAX_MSG_NUM      3
#define  IMPLEMENTATION  FIFO

// SMS Settings
#define  SMS_RETRY_CNT    5

// Our Message Structure
typedef struct message {
  char  to[11];
  char  text[MAX_MSG_LEN];
  int   retry_cnt = 1;
} Message;

Queue  msgQueue(sizeof(Message), MAX_MSG_NUM, IMPLEMENTATION); // Instantiate queue

// SerialCommand
char replybuffer[MAX_CMD_LEN];
SerialCommand SCmd;

// DHT11 Sensor
SimpleDHT11 dht11;

// FONA
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX,FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

// Timing
static unsigned long   trhMillis                = 0;             // Time interval tracking
static unsigned long   previousLedMillis        = 0; 
static unsigned long   ledFlashInterval         = 500; 
static unsigned long   updatePreviousMillis     = 0;     // last time update
static int             updateInterval           = 5;     // How often to check sensors 5 seconds
bool                   firstCheck               = true;

// LED States:
int ledGreenState = LOW;
int ledRedState = LOW;
int ledOrangeState = LOW;

// Our current State
struct STATE
{
  byte  tempState    ;
  float temp         ;
  
  byte  voltageState ;
  float voltageIn    ;
  float voltageBatt  ;
  
  
  byte  humidityState;
  int   humidity     ;

  float lastAlert = 0;
  bool  alarm        ;
  bool  stateChange  ;
};

STATE current{};
STATE previous{};

// Our Config Structure
struct config_t
{
    int   tempHigh;
    int   tempLow;
    int   voltageHigh;
    int   voltageLow;
    int   humidityHigh;
    int   humidityLow;    
    long  alarmRepeat;
    char  phone[3][11];
    bool  wireless_en = true;
    bool  debug = false;
} configuration;

void setup()
{   

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  led(ORANGE);
  //Read our last state from EEPROM
  EEPROM.get(EEPROMStart, configuration);

  Serial.begin(9600);
  
  if(configuration.debug){
    Serial.println(F("DEBUG: BEGIN"));
  }
  Serial.println(F("Starting 3GAlarmPanel-NDM.guru V0001"));
  Serial.println(F("Line termination needs to be CR for terminal to work..."));
  
  if(configuration.wireless_en){
    if(configuration.debug){
      Serial.println(F("DEBUG: Wireless Enabled, starting"));
    }
    Serial.println(F("Initializing wireless...."));
    pinMode(FONA_PWR, OUTPUT);
    
    digitalWrite(FONA_PWR, HIGH);
    delay(4000);
    digitalWrite(FONA_PWR, LOW);
    
    fonaSerial->begin(4800);
    if (! fona.begin(*fonaSerial)) {
      Serial.println(F("Couldn't find FONA"));
      while (1);
    }
  }
  
  // Setup callbacks for SerialCommand commands   
  SCmd.addCommand("temp",setTemp);
  SCmd.addCommand("humidity",setHumidity);
  SCmd.addCommand("wireless",setWireless);
  SCmd.addCommand("debug",setDebug);
  SCmd.addCommand("phone",setPhone);
  SCmd.addCommand("repeat",setRepeat);
  SCmd.addCommand("voltage",setVoltage);
  SCmd.addCommand("show",showCurrent);
  SCmd.addCommand("config",showConfig);
  SCmd.addCommand("default",clearEeprom);
  SCmd.addCommand("network",showNetworkStatus);
  SCmd.addDefaultHandler(unrecognized);

  if(configuration.debug){
    showConfig();
  }
    
  Serial.println(F("Ready"));
  Serial.println(F("Current Status:"));
  updateStatus();
  showCurrent();
 }

void loop()
{  
  updateLeds();

  unsigned long currentMillis = millis();

  if(currentMillis - updatePreviousMillis > (updateInterval * 1000)) {
     if(configuration.debug){
       Serial.println(F("DEBUG: Update interval reached"));
     }
     updatePreviousMillis = currentMillis;
     if(configuration.debug){
      showFree();
     }
       
     updateStatus();
     // We need to check for new alarms each check, and send those here - Think this is the best place...
     if(current.stateChange){
      queueAlerts();     
     }
     // Push out sms queue here
     sendAlerts();
  }

  // Handles repeat Alert Messages
  if((currentMillis - current.lastAlert > configuration.alarmRepeat) or firstCheck) {
      firstCheck = false;
       if(configuration.debug){
         Serial.println(F("DEBUG: Alarm repeat interval reached"));
          if(!current.alarm){
           Serial.println(F("DEBUG: No Alarms to send"));
      };
       }
      current.lastAlert = currentMillis;

      if(current.alarm){
        queueAlerts();
      };
  }

  // Read the serial buffer and run commands
  SCmd.readSerial();     
}
