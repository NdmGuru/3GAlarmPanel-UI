#include <SHT1x.h>
#include <Adafruit_FONA.h>
#include <SoftwareSerial.h> 
#include <SerialCommand.h>
#include <EEPROM.h>

// General Vars
#define arduinoLED 13   // Arduino LED on board
#define EEPROMStart 513 // Start write and read for EEPROM
#define NUM_SAMPLES 10  // number of analog samples to take per voltage reading

// SerialCommand
char replybuffer[64];
SerialCommand SCmd;

// FONA Settings
#define FONA_TX 2
#define FONA_RX 3
#define FONA_RST 9

// Specify data and clock connections and instantiate SHT1x object
#define SHT11_DATA  5
#define SHT11_CLOCK 4

SHT1x sht1x(SHT11_DATA, SHT11_CLOCK);

// FONA
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX,FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

// Timing
unsigned long   alarmPreviousMillis      = 0; // last time update
long            alarmCheckInterval       = 5000; // 5 Seconds
unsigned long   alertPreviousMillis      = 0; // last time update
long            alertCheckInterval       = 6000; // 6 Seconds
bool            alertFirstCheck          = true;


struct alarm_t
{
  bool    current        = false;
  long    lastAlert      = 0    ;
  bool    tempLowCurrent = false;
  int     tempHighCurrent       ;
  int     temp                  ;
  float   voltage               ;
  bool    voltageCurrent        ;
  bool    muted          = false;
} alarm;

struct current_t
{
  float temp_c;
  float temp_f;
  float humidity;
  float voltage;
} current;

// Our Config Structure
struct config_t
{
    int   alertHigh;
    int   alertLow;
    bool  tempAlarm;
    long  alarmRepeat;
    char  phone1[11];
    char  phone2[11];
    bool  wireless_en = false;
} configuration;

void setup()
{   
  //Read our last state from EEPROM
  EEPROM.get(EEPROMStart, configuration);

  Serial.begin(9600); 
  Serial.println(F("Starting 3GAlarmPanel-NDM.guru V0001"));
  Serial.println(F("Line termination needs to be CR for terminal to work..."));
  
  if(configuration.wireless_en){
    Serial.println(F("Initializing wireless....(May take 3 seconds)"));
    fonaSerial->begin(4800);
    if (! fona.begin(*fonaSerial)) {
      Serial.println(F("Couldn't find FONA"));
      while (1);
    }
  }
  
  // Setup callbacks for SerialCommand commands   
  SCmd.addCommand("temp",setTemp);
  SCmd.addCommand("show",showCurrent);
  SCmd.addCommand("phone",setPhone);
  SCmd.addCommand("config",showConfig);
  SCmd.addCommand("repeat",setRepeat);
  SCmd.addCommand("default",clearEeprom);
  SCmd.addCommand("testsms",sendTestMessage);
  SCmd.addDefaultHandler(unrecognized);
  
  showConfig();
  
  Serial.println(F("Ready")); 
 }

void loop()
{  
  // Basic timing here, may need to get ALOT more complicated...
  unsigned long currentMillis = millis();

  if(alertFirstCheck){
    updateStatus();
    sendAlerts(currentMillis);
    alertFirstCheck = false;
  }

  if(currentMillis - alarmPreviousMillis > alarmCheckInterval) {
     alarmPreviousMillis = currentMillis;
     updateStatus();
  }

  if(currentMillis - alertPreviousMillis > alertCheckInterval) {
     alertPreviousMillis = currentMillis;
     sendAlerts(currentMillis);
  }

  // Read the serial buffer and run commands
  SCmd.readSerial();     
}

void sendAlerts(unsigned long currentMillis ){
  //unsigned long currentMillis = millis();
  if(alarm.current == true){
    
    Serial.println("Alarms are currenet");
    
    if(currentMillis - alarm.lastAlert > configuration.alarmRepeat) {
         alarm.lastAlert = currentMillis;
         
         Serial.println("Sending Alerts");
         // Show to the term
         showCurrent();
          
         // Send SMS's
         smsAlerts();}
  }
}

void smsAlerts(){
  // send the sms alerts
  Serial.println("We would sms here");
}

void updateStatus(){
  // Update the temp/humidity sensors
  readSHT11();
  readVoltage();
  
  // Update high temp alarm status
  if(current.temp_c >= configuration.alertHigh){
    alarm.temp = current.temp_c;
    alarm.tempHighCurrent = true;
  }else{
    alarm.tempHighCurrent = false;
  }

  // Update low temp alarm status
  if(current.temp_c <= configuration.alertLow){
    alarm.temp = current.temp_c;
    alarm.tempLowCurrent = true;
  }else{
    alarm.tempLowCurrent = false;
  } 

  // Update low temp alarm status
  if(current.voltage <= 4.00){
    alarm.voltage = current.voltage;
    alarm.voltageCurrent = true;
  }else{
    alarm.voltageCurrent = false;
  }

  // IMPORTANT - ALL ALARM DECITIONS ARE MADE ON THIS VAR!! UPDATE THE FUCKING THING WHEN U ADD MORE ALARMS!
  // Current - TempHigh, TempLow
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //update the global alarm status var.
  if ((alarm.tempLowCurrent == true) or (alarm.tempHighCurrent == true) or (alarm.voltageCurrent == true)){
    alarm.current = true;
  }else{
    alarm.current = false;
  }
}

void showCurrent(){
  // Print the values to the serial port - Menu triggered
  Serial.print(F("Temperature: "));
  Serial.print(current.temp_c, DEC);
  Serial.print(F("C"));
  Serial.print(F(" Humidity: "));
  Serial.print(current.humidity, DEC);
  Serial.print(F("%"));
  Serial.print(F(" Voltage: "));
  Serial.print(current.voltage, DEC);
  Serial.println(F("V"));
  
  if(alarm.tempHighCurrent == true){
    Serial.print(F("ALARM TEMP HIGH: "));
    Serial.println(alarm.temp);
  }else if(alarm.tempLowCurrent == true){
    Serial.print(F("ALARM TEMP LOW: "));
    Serial.println(alarm.temp);
  }

  if(alarm.voltageCurrent == true){
    Serial.print(F("INPUT VOLTAGE LOW: "));
    Serial.println(alarm.voltage);
  }
    
}


void setRepeat()    
{ 
  char *arg; 

  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    configuration.alarmRepeat=atoi(arg);    // Converts a char string to an integer
    configuration.alarmRepeat=configuration.alarmRepeat * 60000 ;
    Serial.print(F("Repeat: ")); 
    Serial.println(configuration.alarmRepeat / 60000); 
  } 
  else {
    Serial.println(F("repeat <time in minutes>")); 
    return;
  }

  EEPROM.put(EEPROMStart, configuration);
}

void setTemp()    
{ 
  char *arg; 

  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    configuration.alertHigh=atoi(arg);    // Converts a char string to an integer
    Serial.print(F("HIGH: ")); 
    Serial.println(configuration.alertHigh); 
  } 
  else {
    Serial.println(F("TEMP <HIGH> <LOW>")); 
    return;
  }

  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    configuration.alertLow=atol(arg); 
    Serial.print(F("LOW: ")); 
    Serial.println(configuration.alertLow); 
  } 
  else {
    Serial.println(F("TEMP <HIGH> <LOW>"));
    return;
  }
  EEPROM.put(EEPROMStart, configuration);
}

void readSHT11(){
  current.temp_c    = sht1x.readTemperatureC();
  current.temp_f    = sht1x.readTemperatureF();
  current.humidity  = sht1x.readHumidity();
}

void setPhone()    
{ 
  char arg[11];
  char phone1[11];
  char phone2[11];

  strlcpy(phone1, SCmd.next(),11);
  strlcpy(phone2, SCmd.next(),11);
  
 if(strlen(phone1) == 0 or strlen(phone2) == 0){
    Serial.println(F("phone <phone1> <phone2>"));
    return;
  }
  
  strlcpy(configuration.phone1, phone1, 11);
  strlcpy(configuration.phone2, phone2, 11);
  
  EEPROM.put(EEPROMStart, configuration);

  Serial.println(F("Phone numbers saved"));
  showConfig();
}

void sendTestMessage()    
{ 
  char arg[25];
  char message[25] = "Testing message!";
 
 if(configuration.wireless_en){
   if(strlen(message) == 0){
      Serial.println(F("send <message (25char)>"));
      return;
    }
    
    if (!fona.sendSMS(configuration.phone1, message)) {
      Serial.println(F("Failed send message to phone 1"));
    } else {
      Serial.println(F("Sent to phone 1!"));
    }
  
    if (!fona.sendSMS(configuration.phone2, message)) {
      Serial.println(F("Failed send message to phone 2"));
    } else {
      Serial.println(F("Sent to phone 2!"));
    }
  }
}

void showConfig(){
  Serial.println(F("Reading Config from EEPROM:")); 
  EEPROM.get(EEPROMStart, configuration);
  Serial.println(F("Current Config:")); 
  Serial.print(F("    Alert High: ")); 
  Serial.println(configuration.alertHigh); 
  Serial.print(F("    Alert Low: ")); 
  Serial.println(configuration.alertLow);
  Serial.print(F("    Phone 1: ")); 
  Serial.println(configuration.phone1); 
  Serial.print(F("    Phone 2: ")); 
  Serial.println(configuration.phone2);
  Serial.print(F("    Repeat: ")); 
  Serial.println(configuration.alarmRepeat / 6000);

}

void clearEeprom(){
   for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  Serial.println(F("eepromCleared"));
}

void imei(){
  if(configuration.wireless_en){
    // Print module IMEI number.
    char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
    uint8_t imeiLen = fona.getIMEI(imei);
    if (imeiLen > 0) {
      Serial.print(F("Module IMEI: ")); Serial.println(imei);
    }
  }else{
    Serial.println("Enable wireless to show imei");
  }
}

void readVoltage(){
    int sum = 0;                    // sum of samples taken
    unsigned char sample_count = 0;
    float voltage = 0.0;  
  
     while (sample_count < NUM_SAMPLES) {
        sum += analogRead(A0);
        sample_count++;
        delay(10);
    }

    voltage = ((float)sum / (float)NUM_SAMPLES * 5.015) / 1024.0;
    current.voltage = voltage * 11.132;
    
    sample_count = 0;
    sum = 0;
}

// This gets set as the default handler, and gets called when no other command matches. 
void unrecognized()
{
  Serial.println(F("Commands:")); 
  Serial.println(F("  phone <number1> <number2>       # SMS Capable, mobiles")); 
  Serial.println(F("  temp <high> <low>               # 2 didgits")); 
  Serial.println(F("  config                          # Shows config")); 
  Serial.println(F("  default                         # Clears EEProm")); 
}


