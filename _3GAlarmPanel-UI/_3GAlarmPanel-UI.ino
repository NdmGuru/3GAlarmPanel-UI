#include <SoftwareSerial.h> 
#include <SerialCommand.h>
#include <EEPROM.h>

#define arduinoLED 13   // Arduino LED on board
#define EEPROMStart 513 // Start write and read for EEPROM

#define TX 1
#define RX 0
SoftwareSerial SoftSerial = SoftwareSerial(RX,TX);     // The SoftwareSerial Object
SerialCommand SCmd(SoftSerial);   // The demo SerialCommand object, using the SoftwareSerial Constructor

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

  // Setup callbacks for SerialCommand commands   
  SCmd.addCommand("temp",setTemp);  // Converts two arguments to integers and echos them back 
  SCmd.addCommand("phone",setPhone);  // Converts two arguments to integers and echos them back 
  SCmd.addCommand("config",showConfig);  // Converts two arguments to integers and echos them back 
  SCmd.addCommand("default",clearEeprom);  // Converts two arguments to integers and echos them back 
  SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?") 

  Serial.println("Starting 3GAlarmPanel-NDM.guru V0001");
  showConfig();
    
  Serial.println("Ready"); 
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
    Serial.print("HIGH: "); 
    Serial.println(configuration.alertHigh); 
  } 
  else {
    Serial.println("TEMP <HIGH> <LOW>"); 
    return;
  }

  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    configuration.alertLow=atol(arg); 
    Serial.print("LOW: "); 
    Serial.println(configuration.alertLow); 
  } 
  else {
    Serial.println("TEMP <HIGH> <LOW>");
    return;
  }
  EEPROM.put(EEPROMStart, configuration);
}

void setPhone()    
{ 
  char arg[11];
  char phone1[11];
  char phone2[11];

  strlcpy(phone1, SCmd.next(),sizeof(SCmd.next()));
  strlcpy(phone2, SCmd.next(),sizeof(SCmd.next()));
  
 if((phone1 == NULL) || (phone2 == NULL)){
    Serial.println("phone <phone1> <phone2>");
    return;
  }
  
  strlcpy(configuration.phone1, phone1, 11);
  strlcpy(configuration.phone2, phone2, 11);
  
  EEPROM.put(EEPROMStart, configuration);
  showConfig();
}

void showConfig(){
  Serial.println("Reading Config from EEPROM:"); 
  EEPROM.get(EEPROMStart, configuration);
  Serial.println("Current Config:"); 
  Serial.print("    Alert High: "); 
  Serial.println(configuration.alertHigh); 
  Serial.print("    Alert Low: "); 
  Serial.println(configuration.alertLow);
  Serial.print("    Phone 1: "); 
  Serial.println(configuration.phone1); 
  Serial.print("    Phone 2: "); 
  Serial.println(configuration.phone2);
}

void clearEeprom(){
   for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  Serial.print("eepromCleared");
}


// This gets set as the default handler, and gets called when no other command matches. 
void unrecognized()
{
  Serial.println("3GAlarmPanel-ndm.guru V001"); 
}


