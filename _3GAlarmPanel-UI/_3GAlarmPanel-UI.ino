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
unsigned long   alertPreviousMillis      = 0;     // last time update
long            alertInterval            = 5000;  // How often
unsigned long   updatePreviousMillis     = 0;     // last time update
long            updateInterval           = 5000;  // 6 seconds
bool            firstCheck               = true;

struct STATE
{
  bool  tempHigh     ;
  bool  tempLow      ;
  float temp         ;
  
  bool  voltageHigh  ;
  bool  voltageLow   ;
  float voltage      ;
  
  bool  humidityHigh ;
  bool  humidityLow  ;
  bool  humidity     ;

  float lastAlert    ;
  char  message[40]  ;
  bool  alarm        ;
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
    long  alarmRepeat;
    char  phone1[11];
    char  phone2[11];
    bool  wireless_en = true;
} configuration;

void setup()
{   
  //Read our last state from EEPROM
  EEPROM.get(EEPROMStart, configuration);

  Serial.begin(9600); 
  Serial.println(F("Starting 3GAlarmPanel-NDM.guru V0001"));
  Serial.println(F("Line termination needs to be CR for terminal to work..."));
  
  if(configuration.wireless_en){
    
    pinMode(8, OUTPUT);
    Serial.println(F("Initializing wireless....(May take 3 seconds)"));
    
    digitalWrite(8, HIGH);
    delay(4000);
    digitalWrite(8, LOW);
    
    fonaSerial->begin(4800);
    if (! fona.begin(*fonaSerial)) {
      Serial.println(F("Couldn't find FONA"));
      while (1);
    }
  }
  
  // Setup callbacks for SerialCommand commands   
  SCmd.addCommand("temp",setTemp);
  SCmd.addCommand("wireless",setWireless);
  SCmd.addCommand("phone",setPhone);
  SCmd.addCommand("repeat",setRepeat);
  SCmd.addCommand("voltage",setVoltage);
  SCmd.addCommand("show",showCurrent);
  SCmd.addCommand("config",showConfig);
  SCmd.addCommand("default",clearEeprom);
  SCmd.addCommand("testsms",sendTestMessage);
  SCmd.addDefaultHandler(unrecognized);
  
  showConfig();
  updateStatus();
  
  Serial.println(F("Ready"));
  Serial.println(F("Current Status:"));
  showCurrent();
 }

void loop()
{  
  // Basic timing here, may need to get ALOT more complicated...
  unsigned long currentMillis = millis();

  if(firstCheck){
    updateStatus();
    updateAlertString();
    sendAlerts(currentMillis);
    firstCheck = false; // We've sent on our first run now.  
  }

  if(currentMillis - updatePreviousMillis > updateInterval) {
     updatePreviousMillis = currentMillis;
     updateStatus();
  }

  if(currentMillis - alertPreviousMillis > alertInterval) {
     alertPreviousMillis = currentMillis;
     sendAlerts(currentMillis);
  }

  // Read the serial buffer and run commands
  SCmd.readSerial();     
}

void sendAlerts(unsigned long currentMillis ){
  
  if(current.alarm == true){      
    if((currentMillis - current.lastAlert > configuration.alarmRepeat) or (firstCheck)) {
         current.lastAlert = currentMillis;
         
         Serial.println(F("Sending Alerts"));
         // Show to the term
         Serial.println(current.message);          
         // Send SMS's
         smsAlerts();
         
     }
  }
}

void updateAlertString(){
  unsigned long currentMillis = millis();
  char message[40] = "";
  char currentTemp[7];
  char currentVoltage[7];
  bool sendNow = false;
  
  dtostrf(current.temp, 5, 2, currentTemp);
  dtostrf(current.voltage, 5, 2, currentVoltage);

  // Alarms that were previousaly high
  if ((current.tempHigh) and (previous.tempHigh)){
      strcat(message,"TEMP HIGH:");
      strcat(message, currentTemp);
      strcat(message, " ");
  }else if((current.tempLow) and (previous.tempLow)){
      strcat(message,"TEMP LOW:");
      strcat(message, currentTemp);
      strcat(message, " ");
  }else if((previous.tempLow) or (previous.tempHigh)){
      strcat(message,"TEMP OK:");
      strcat(message, currentTemp);
      strcat(message, " ");
  }


  if ((current.voltageHigh) and (previous.voltageHigh)){
      strcat(message,"VOLTAGE HIGH:");
      strcat(message, currentVoltage);
      strcat(message, " ");
  }else if((current.voltageLow) and (previous.voltageLow)){
      strcat(message,"VOLTAGE LOW:");
      strcat(message, currentVoltage);
      strcat(message, " ");
  }else if((previous.voltageLow) or (previous.voltageHigh)){
      strcat(message,"VOLTAGE OK:");
      strcat(message, currentVoltage);
      strcat(message, " ");
      sendNow = true;
  }
   strcpy(current.message,message);
   if(sendNow == true){
    sendAlerts(currentMillis);
   }
}

void smsAlerts(){
 if(configuration.wireless_en == true){
   if(strlen(current.message) == 0){
      Serial.println(F("Message empty, skipping"));
      return;
    }
    
    if (!fona.sendSMS(configuration.phone1, current.message)) {
      Serial.println(F("Failed send message to phone 1"));
    } else {
      Serial.println(F("Sent to phone 1!"));
    }
  
    if (!fona.sendSMS(configuration.phone2, current.message)) {
      Serial.println(F("Failed send message to phone 2"));
    } else {
      Serial.println(F("Sent to phone 2!"));
    }
  }else{
    Serial.println(F("Wireless Disabled, surpessing sms output"));
  }
}

void updateStatus(){
  // Update the temp/humidity sensors
  readSHT11();
  readVoltage();
  
  // Copy current state to previous state
  previous = current;
  
  // Update high temp alarm status
  if(current.temp >= configuration.tempHigh){
    current.tempHigh = true;
  }else{
    current.tempHigh = false;
  }

  // Update low temp alarm status
  if(current.temp <= configuration.tempLow){
    current.tempLow = true;
  }else{
    current.tempLow = false;
  } 

  // Update voltage alarm status
  if(current.voltage <= configuration.voltageLow){
    current.voltageLow = true;
  }else{
    current.voltageLow = false;
  }

  // Update voltage alarm status
  if(current.voltage >= configuration.voltageHigh){
    current.voltageHigh = true;
  }else{
    current.voltageHigh = false;
  }

  // IMPORTANT - ALL ALARM DECITIONS ARE MADE ON THIS VAR!! UPDATE THE FUCKING THING WHEN U ADD MORE ALARMS!
  // Current - TempHigh, TempLow, Voltage
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //update the global alarm status var.
  if ((current.tempLow == true) or (current.tempHigh == true) or (current.voltageLow == true) or (current.voltageHigh == true)){
    current.alarm = true;
  }else{
    current.alarm = false;
  }
  // Update the alert String.
  updateAlertString();
}

void showCurrent(){
  // Print the values to the serial port - Menu triggered
  Serial.print(F("Temperature: "));
  Serial.print(current.temp, DEC);
  Serial.print(F("C"));
  Serial.print(F(" Humidity: "));
  Serial.print(current.humidity, DEC);
  Serial.print(F("%"));
  Serial.print(F(" Voltage: "));
  Serial.print(current.voltage, DEC);
  Serial.println(F("V")); 
  Serial.print(F("Message: "));
  Serial.println(current.message);
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

void setWireless()    
{ 
  char *arg;
  int bool_en;

  arg = SCmd.next(); 
  if (arg != NULL) {
   bool_en=atoi(arg);    
    if(bool_en == 1){
      configuration.wireless_en = true;
      Serial.println(F("Wireless Enabled"));
    }else if(bool_en == 0){
      configuration.wireless_en = false;
      Serial.println(F("Wireless Disabled"));
    }
  } else {
    Serial.println(F("wireless <bool>")); 
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
    configuration.tempHigh=atoi(arg);    // Converts a char string to an integer
    Serial.print(F("HIGH: ")); 
    Serial.println(configuration.tempHigh); 
  } 
  else {
    Serial.println(F("TEMP <HIGH> <LOW>")); 
    return;
  }

  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    configuration.tempLow=atol(arg); 
    Serial.print(F("LOW: ")); 
    Serial.println(configuration.tempLow); 
  } 
  else {
    Serial.println(F("TEMP <HIGH> <LOW>"));
    return;
  }
  EEPROM.put(EEPROMStart, configuration);
}

void setVoltage()    
{ 
  char *arg; 

  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    configuration.voltageLow=atoi(arg);    // Converts a char string to an integer
    Serial.print(F("LOW: ")); 
    Serial.println(configuration.voltageLow); 
  } 
  else {
    Serial.println(F("VOLTAGE <LOW> <HIGH>")); 
    return;
  }

  arg = SCmd.next(); 
  if (arg != NULL) 
  {
    configuration.voltageHigh=atol(arg); 
    Serial.print(F("HIGH: ")); 
    Serial.println(configuration.voltageHigh); 
  } 
  else {
    Serial.println(F("VOLTAGE <LOW> <HIGH>"));
    return;
  }
  EEPROM.put(EEPROMStart, configuration);
}



void readSHT11(){
  current.temp    = sht1x.readTemperatureC();
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
  Serial.print(F("    Temp High: ")); 
  Serial.println(configuration.tempHigh); 
  Serial.print(F("    Temp Low: ")); 
  Serial.println(configuration.tempLow);
  Serial.print(F("    Voltage High: ")); 
  Serial.println(configuration.voltageHigh); 
  Serial.print(F("    Voltage Low: ")); 
  Serial.println(configuration.voltageLow);
  Serial.print(F("    Phone 1: ")); 
  Serial.println(configuration.phone1); 
  Serial.print(F("    Phone 2: ")); 
  Serial.println(configuration.phone2);
  Serial.print(F("    Repeat: ")); 
  Serial.println(configuration.alarmRepeat / 60000);

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
    Serial.println(F("Enable wireless to show imei"));
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
  Serial.println(F("  phone <number1> <number2>")); 
  Serial.println(F("  temp <high> <low>")); 
  Serial.println(F("  voltage <low> <high>")); 
  Serial.println(F("  repeat <minutes>")); 
  Serial.println(F("  config")); 
  Serial.println(F("  show")); 
  Serial.println(F("  testsms")); 
  Serial.println(F("  default")); 
}

