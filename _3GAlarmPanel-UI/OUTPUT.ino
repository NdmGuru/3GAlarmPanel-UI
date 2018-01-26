void sendAlertString(){
  if(configuration.debug){
   Serial.println(F("DEBUG: Update Message String"));
 }
  unsigned long currentMillis = millis();
  char message_t[MAX_MSG_LEN] = "";
  // These are 7 in length because we can have "-00.00\n"
  // This was a major cause of memory overwrite!
  char currentTemp[7] = "";
  char currentVoltage[7] = "";
  char currentHumidity[7] = "";
  
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
  if(configuration.debug){
    showFree();
  }
  Message message;
  strcpy(message.text, message_t);
  msgQueue.push(&message);  
}

void sendMsgs(){
  Message out_message ;
  int retryCount = 0;
  
  if(configuration.debug){
    Serial.println(F("DEBUG: Send Messages"));
    if(msgQueue.isEmpty()){
      Serial.println(F("DEBUG: No messages to send"));
    }
  }
  if(configuration.wireless_en){
    while (!msgQueue.isEmpty()){
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
    showFree();
    if(!msgQueue.isEmpty()){
      Serial.println(F("Wireless Disabled, here's the message"));
      Serial.print(F("SENDING MESSAGE: "));
      while (!msgQueue.isEmpty()){
        msgQueue.pop(&out_message);
        Serial.println(out_message.text);
      }
    }
  }
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
  Serial.println(configuration.phone1); 
  Serial.print(F("    Phone 2: ")); 
  Serial.println(configuration.phone2);
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

void LED(byte colour){
  switch(colour){
    //Green
    case B000000:
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_GREEN, LOW);
      break;
    case B000001:
      digitalWrite(LED_GREEN, HIGH);
      break;
    case B000010:
      digitalWrite(LED_RED, HIGH);
      break;
    case B000011:
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, HIGH);
      break;
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

