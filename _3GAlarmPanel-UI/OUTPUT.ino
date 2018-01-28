void showCurrent(){
  // Print the values to the serial port - Menu triggered
  Serial.print(F("Temperature: ")); 
  Serial.print(current.temp, 0); // NDM - no point with decimal points with DHT11
  Serial.print(F("C"));
  Serial.print(F(" Humidity: ")); 
  Serial.print(current.humidity);
  Serial.print(F("%"));
  Serial.print(F(" VoltageIn: "));
  Serial.print(current.voltageIn);
  Serial.print(F("V"));
  Serial.print(F(" VoltageBatt: "));
  Serial.print(current.voltageBatt);
  Serial.println(F("V")); 

}

// This gets set as the default handler, and gets called when no other command matches. 
void unrecognized()
{ 
  Serial.println(F("Commands:")); 
  Serial.println(F("  phone     <pos>  <number>")); 
  Serial.println(F("  temp      <high> <low>")); 
  Serial.println(F("  humidity  <high> <low>")); 
  Serial.println(F("  voltage   <high> <low>")); 
  Serial.println(F("  repeat    <minutes>")); 
  Serial.println(F("  wireless  <bool>")); 
  Serial.println(F("  debug     <bool>"));
  Serial.println(F("  config")); 
  Serial.println(F("  show")); 
  Serial.println(F("  state")); 
  Serial.println(F("  network"));  
  Serial.println(F("  default")); 
}

void showFree(){
  Serial.print(F("DEBUG: Free Memory="));
  Serial.println(freeMemory());
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
  Serial.println(configuration.phone[0]); 
  Serial.print(F("    Phone 2: ")); 
  Serial.println(configuration.phone[1]);
  Serial.print(F("    Phone 3: ")); 
  Serial.println(configuration.phone[2]);
  Serial.print(F("    Repeat: ")); 
  Serial.println(configuration.alarmRepeat / 60000);
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

void led(byte colour){
  switch(colour){
    case OFF:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);
      break;
    case GREEN:
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
      break;
    case RED:
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      break;
    case ORANGE:
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, HIGH);
      break;
   }
}

void updateLeds(){
  if(current.alarm){
    led(RED);
    return;;
  }else{
    led(GREEN);
    return;
  }
}

void blink(byte colour){
  unsigned long currentMillis = millis();

  if (currentMillis - previousLedMillis >= ledFlashInterval) {
    // save the last time you blinked the LED
    previousLedMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:


    switch(colour){
      case B000001:
        if (ledGreenState == LOW) {
          ledGreenState = HIGH;
        } else {
          ledGreenState = LOW;
        }
        digitalWrite(LED_GREEN, ledGreenState);
        break;
      case B000010:
        if (ledRedState == LOW) {
          ledRedState = HIGH;
        } else {
          ledRedState = LOW;
        }
        digitalWrite(LED_RED, ledRedState);
        break;
      case B000011:
         if (ledOrangeState == LOW) {
          ledOrangeState = HIGH;
        } else {
          ledOrangeState = LOW;
        }
        digitalWrite(LED_GREEN, ledOrangeState);
        digitalWrite(LED_RED, ledOrangeState);
        break;
      }
  }
}

void showState(){
  char message_text[MAX_MSG_LEN] = "";
  
  buildAlertString(message_text);
  Serial.println(message_text);
}
