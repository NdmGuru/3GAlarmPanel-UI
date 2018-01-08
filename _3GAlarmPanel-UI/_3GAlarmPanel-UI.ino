#include <cppQueue.h>
#include <MemoryFree.h>
#include <SHT1x.h>
#include <Adafruit_FONA.h>
#include <SoftwareSerial.h> 
#include <SerialCommand.h>
#include <EEPROM.h>

// General Vars
#define arduinoLED 13   // Arduino LED on board
#define EEPROMStart 513 // Start write and read for EEPROM
#define NUM_SAMPLES 10  // number of analog samples to take per voltage reading

// Queue for SMS Messages
#define  IMPLEMENTATION  FIFO

typedef struct message {
  char*    text;
  bool     sent;
} Message;

Queue  msgQueue(80, 5, IMPLEMENTATION); // Instantiate queue

// SerialCommand
char replybuffer[32];
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

// Timing
static unsigned long   updatePreviousMillis     = 0;     // last time update
static int             updateInterval           = 5;     // How often to check sensors 5 seconds
bool                   firstCheck               = true;

// General Control
#define DEBUG true

// 
struct STATE
{
  byte  tempState    ;
  int   temp         ;
  
  byte  voltageState ;
  int   voltage      ;
  
  byte  humidityState;
  int   humidity     ;

  float lastAlert = 0; // High so we trigger on first alert?
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
    char  phone1[11];
    char  phone2[11];
    bool  wireless_en = true;
    bool  debug = false;
} configuration;

void setup()
{   

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
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    delay(4000);
    digitalWrite(8, LOW);
    delay(2000);
    
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
  
  // Basic timing here, may need to get ALOT more complicated...
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
      sendAlertString();     
     }
     sendMsgs();
  }

  // Handles repear Alert Messages
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
        sendAlertString();
      };
  }

  // Read the serial buffer and run commands
  SCmd.readSerial();     
}

void sendAlertString(){
  if(configuration.debug){
   Serial.println(F("DEBUG: Update Message String"));
 }
  unsigned long currentMillis = millis();
  char message_t[52] = "";
  char currentTemp[6] = "";
  char currentVoltage[6] = "";
  char currentHumidity[6] = "";
  
  dtostrf(current.temp, 5, 2, currentTemp);
  dtostrf(current.voltage, 5, 2, currentVoltage);
  dtostrf(current.humidity, 5, 2, currentHumidity);

  switch(current.tempState){
    case B00000000:
      strcat(message_t,"TEMP OK:");
      strcat(message_t, currentTemp);
      strcat(message_t, " ");
      break;
    case B00000001:
      strcat(message_t,"TEMP LOW:");
      strcat(message_t, currentTemp);
      strcat(message_t, " ");
      break;
    case B00000010:
      strcat(message_t,"TEMP HIGH:");
      strcat(message_t, currentTemp);
      strcat(message_t, " ");
      break;
  }

  switch(current.humidityState){
    case B00000000:
      strcat(message_t,"HUMIDITY OK:");
      strcat(message_t, currentHumidity);
      strcat(message_t, " ");
      break;
    case B00000001:
      strcat(message_t,"HUMIDITY LOW:");
      strcat(message_t, currentHumidity);
      strcat(message_t, " ");
      break;
    case B00000010:
      strcat(message_t,"HUMIDITY HIGH:");
      strcat(message_t, currentHumidity);
      strcat(message_t, " ");
      break;
  }

  
  switch(current.voltageState){
    case B00000000:
      strcat(message_t,"VOLTAGE OK:");
      strcat(message_t, currentVoltage);
      strcat(message_t, " ");
      break;
    case B00000001:
      strcat(message_t,"VOLTAGE LOW:");
      strcat(message_t, currentVoltage);
      strcat(message_t, " ");
      break;
    case B00000010:
      strcat(message_t,"VOLTAGE HIGH:");
      strcat(message_t, currentVoltage);
      strcat(message_t, " ");
      break;
  }
  
  strcat(message_t,0);
  if(configuration.debug){
    Serial.print(F("DEBUG: MESSAGE="));
    Serial.println(message_t);
  }
  showFree();

  Message message = {message_t, false};
  msgQueue.push(&message);  
  
}

void sendMsgs(){
  int retryCount = 0;
  if(configuration.debug){
    Serial.println(F("DEBUG: Send Messages"));
    if(msgQueue.isEmpty()){
      Serial.println(F("DEBUG: No messages to send"));
    }
  }
  if(configuration.wireless_en){
    while (!msgQueue.isEmpty()){
      Message out_message;
      msgQueue.peek(&out_message);
      if(configuration.debug){
        Serial.print(F("SENDING MESSAGE: "));
        Serial.println(out_message.text);
        showFree();
      } 
      while(retryCount < 3){
        retryCount++;
        if (!fona.sendSMS(configuration.phone1, out_message.text)) {
          Serial.println(F("Failed to send to Phone 1!"));
        } else {
          Serial.println(F("Message sent to phone 1!"));
          break;
        }

//        if (!fona.sendSMS(configuration.phone2, out_message.text)) {
//          Serial.println(F("Failed to send to Phone 2!"));
//        } else {
//          Serial.println(F("Message sent to phone 2!"));
//        }
      }
      retryCount=0; // Reset after this message
      msgQueue.pop(&out_message);
    }
  }else{
    // Wireless disabled, tell the console and bail
    if(!msgQueue.isEmpty()){
      Serial.println(F("Wireless Disabled, here's the message"));
      Serial.print(F("SENDING MESSAGE: "));
      while (!msgQueue.isEmpty()){
        Message out_message;
        msgQueue.pop(&out_message);
        Serial.println(out_message.text);
      }
    }
  }
}

void updateStatus(){
  
  if(configuration.debug){
    Serial.println(F("DEBUG: Update Status triggered"));
  }

  // Assume we have no alarms or state change
  current.alarm = false;  
  current.stateChange = false;
  
  // Copy current state to previous state
  previous = current;
  
  // Update the temp/humidity sensors
  readSHT11();
  readVoltage();
  
  // Update TEMP status
  if(current.temp >= configuration.tempHigh){
    current.tempState = B00000010;
    current.alarm = true;
    if(configuration.debug){
       Serial.println(F("DEBUG: TEMP High"));
    }
  }else if (current.temp <= configuration.tempLow){
    current.tempState = B00000001;
    current.alarm = true;
    if(configuration.debug){
       Serial.println(F("DEBUG: TEMP Low"));
    }
  }else{
    current.tempState = B00000000;
    if(configuration.debug){
       Serial.println(F("DEBUG: Temp Normal"));
    }
  }

  // Update HUMIDITY status
  if(current.humidity >= configuration.humidityHigh){
    current.humidityState = B00000010;
    current.alarm = true;
    if(configuration.debug){
       Serial.println(F("DEBUG: HUMIDITY High"));
    }
  }else if (current.humidity <= configuration.humidityLow){
    current.humidityState = B00000001;
    current.alarm = true;
    if(configuration.debug){
       Serial.println(F("DEBUG: HUMIDITY Low"));
    }
  }else{
    current.humidityState = B00000000;
    // Track state change for dismiss messages
    if(configuration.debug){
       Serial.println(F("DEBUG: Humidity Normal"));
    }
  }

  // Update VOLTAGE status
  if(current.voltage >= configuration.voltageHigh){
    current.voltageState = B00000010;
    current.alarm = true;
    if(configuration.debug){
       Serial.println(F("DEBUG: VOLTAGE High"));
    }
  }else if (current.voltage <= configuration.voltageLow){
    current.voltageState = B00000001;
    current.alarm = true;
    if(configuration.debug){
       Serial.println(F("DEBUG: VOLTAGE Low"));
    }
  }else{
    current.voltageState = B00000000;
    if(configuration.debug){
       Serial.println(F("DEBUG: Voltage Normal"));
    }
  }
  
  if (!current.alarm){
    if(configuration.debug){
       Serial.println(F("DEBUG: No Alarms Found"));
    }
  }else{
    if(configuration.debug){
       Serial.println(F("DEBUG: Alarms Present"));
    }
  }

  if((current.tempState != previous.tempState) or (current.humidityState != previous.humidityState) or (current.voltageState != previous.voltageState)){
    current.stateChange = true;
  }else if((current.tempState == previous.tempState) and (current.humidityState == previous.humidityState) and (current.voltageState == previous.voltageState)){
    current.stateChange = false;
  }
  showFree();
  
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

void setDebug()    
{ 
  char *arg;
  int bool_en;

  arg = SCmd.next(); 
  if (arg != NULL) {
   bool_en=atoi(arg);    
    if(bool_en == 1){
      configuration.debug = true;
      Serial.println(F("Debug Enabled"));
    }else if(bool_en == 0){
      configuration.debug = false;
      Serial.println(F("Debug Disabled"));
    }
  } else {
    Serial.println(F("debug <bool>")); 
    return;
  }
  
  EEPROM.put(EEPROMStart, configuration);
}

void setTemp()    
{ 
  char *tempHigh; 
  char *tempLow;

  int tempHigh_t;
  int tempLow_t;
  
  tempHigh = SCmd.next(); 
  tempLow  = SCmd.next();

  if((tempHigh == NULL) or (tempLow == NULL)){
    Serial.println(F("TEMP <HIGH> <LOW>")); 
    return;
  }
  
  tempHigh_t=atoi(tempHigh);    // Converts a char string to an integer
  tempLow_t =atoi(tempLow);    // Converts a char string to an integer
  
  if((tempHigh_t <= tempLow_t)){
    Serial.println(F("Temp rang error")); 
    Serial.println(F("TEMP <HIGH> <LOW>")); 
    return;
  }

  configuration.tempHigh = tempHigh_t;
  configuration.tempLow  = tempLow_t;

  EEPROM.put(EEPROMStart, configuration);
  showConfig();
}

void setVoltage()    
{ 
  char *voltageHigh; 
  char *voltageLow;

  int voltageHigh_t;
  int voltageLow_t;
  
  voltageHigh = SCmd.next(); 
  voltageLow  = SCmd.next();

  if((voltageHigh == NULL) or (voltageLow == NULL)){
    Serial.println(F("VOLTAGE <HIGH> <LOW>")); 
    return;
  }
  
  voltageHigh_t=atoi(voltageHigh);    // Converts a char string to an integer
  voltageLow_t =atoi(voltageLow);    // Converts a char string to an integer
  
  if((voltageHigh_t <= voltageLow_t)){
    Serial.println(F("Voltage range error")); 
    Serial.println(F("VOLTAGE <HIGH> <LOW>")); 
    return;
  }

  configuration.voltageHigh = voltageHigh_t;
  configuration.voltageLow  = voltageLow_t;

  EEPROM.put(EEPROMStart, configuration);
  showConfig();
}
void setHumidity()    
{ 
  char *humidityHigh; 
  char *humidityLow;

  int humidityHigh_t;
  int humidityLow_t;
  
  humidityHigh = SCmd.next(); 
  humidityLow  = SCmd.next();

  if((humidityHigh == NULL) or (humidityLow == NULL)){
    Serial.println(F("HUMIDITY <HIGH> <LOW>")); 
    return;
  }
  
  humidityHigh_t=atoi(humidityHigh);    // Converts a char string to an integer
  humidityLow_t =atoi(humidityLow);    // Converts a char string to an integer
  
  if((humidityHigh_t <= humidityLow_t)){
    Serial.println(F("Humidity rang error")); 
    Serial.println(F("HUMIDITY <HIGH> <LOW>")); 
    return;
  }

  configuration.humidityHigh = humidityHigh_t;
  configuration.humidityLow  = humidityLow_t;

  EEPROM.put(EEPROMStart, configuration);
  showConfig();
}

void readSHT11(){
  if(configuration.debug){
     Serial.println(F("DEBUG: Reading SHT Temp Sensor"));
  }
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

void showConfig(){
  Serial.println(F("Reading Config from EEPROM:")); 
  EEPROM.get(EEPROMStart, configuration);
  Serial.println(F("Current Config:")); 
  Serial.print(F("    Temp High: ")); 
  Serial.println(configuration.tempHigh); 
  Serial.print(F("    Temp Low: ")); 
  Serial.println(configuration.tempLow);
  Serial.print(F("    Humidiry High: ")); 
  Serial.println(configuration.humidityHigh); 
  Serial.print(F("    Humidity Low: ")); 
  Serial.println(configuration.humidityLow);
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

void showNetworkStatus(){
  // read the network/cellular status
  uint8_t n = fona.getNetworkStatus();
  int8_t r;
  uint8_t rssi = fona.getRSSI();
  
  Serial.print(F("Network status "));
  Serial.print(n);
  Serial.print(F(": "));
  if (n == 0) Serial.println(F("Not registered"));
  if (n == 1) Serial.println(F("Registered (home)"));
  if (n == 2) Serial.println(F("Not registered (searching)"));
  if (n == 3) Serial.println(F("Denied"));
  if (n == 4) Serial.println(F("Unknown"));
  if (n == 5) Serial.println(F("Registered roaming"));

  Serial.print(F("RSSI = ")); Serial.print(rssi); Serial.print(": ");
  if (rssi == 0) r = -115;
  if (rssi == 1) r = -111;
  if (rssi == 31) r = -52;
  if ((rssi >= 2) && (rssi <= 30)) {
    r = map(rssi, 2, 30, -110, -54);
  }
  Serial.print(r); Serial.println(F(" dBm"));

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
  Serial.println(F("  humidity <high> <low>")); 
  Serial.println(F("  voltage <high> <low>")); 
  Serial.println(F("  repeat <minutes>")); 
  Serial.println(F("  config")); 
  Serial.println(F("  show")); 
  Serial.println(F("  testsms")); 
  Serial.println(F("  default")); 
}

void showFree(){
  Serial.print(F("DEBUG: Free Memory="));
  Serial.println(freeMemory());
}

