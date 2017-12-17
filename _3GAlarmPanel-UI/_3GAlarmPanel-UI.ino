#include <Adafruit_FONA.h>
#include <SoftwareSerial.h> 
#include <SerialCommand.h>
#include <EEPROM.h>


#define arduinoLED 13   // Arduino LED on board
#define EEPROMStart 513 // Start write and read for EEPROM

// FONA Settings
#define FONA_TX 2
#define FONA_RX 3
#define FONA_RST 9

// this is a large buffer for replies
char replybuffer[64];

#define TX 1
#define RX 0
SoftwareSerial SoftSerial = SoftwareSerial(RX,TX);     // The SoftwareSerial Object
SerialCommand SCmd(SoftSerial);   // The demo SerialCommand object, using the SoftwareSerial Constructor

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX,FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

struct config_t
{
    int alertHigh;
    int alertLow;
    char phone1[11];
    char phone2[11];
} configuration;

void setup()
{   
  //Read our last state from EEPROM
  EEPROM.get(EEPROMStart, configuration);
  pinMode(arduinoLED,OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED,LOW);    // default to LED off

  Serial.begin(9600); 
  SoftSerial.begin(9600); 
  
  Serial.println(F("Starting 3GAlarmPanel-NDM.guru V0001"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  
  // Setup callbacks for SerialCommand commands   
  SCmd.addCommand("temp",setTemp);  // Converts two arguments to integers and echos them back 
  SCmd.addCommand("phone",setPhone);  // Converts two arguments to integers and echos them back 
  SCmd.addCommand("config",showConfig);  // Converts two arguments to integers and echos them back 
  SCmd.addCommand("default",clearEeprom);  // Converts two arguments to integers and echos them back 
  SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?") 
  
  showConfig();
    
  Serial.println(F("Ready")); 
 }

void loop()
{  
  SCmd.readSerial();     // We don't do much, just process serial commands
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
}

void clearEeprom(){
   for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  Serial.print(F("eepromCleared"));
}

void imei(){
  // Print module IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print(F("Module IMEI: ")); Serial.println(imei);
  }
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


