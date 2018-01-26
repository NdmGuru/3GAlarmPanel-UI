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
  if(configuration.debug){
     showFree();
  }else{
    Serial.println("Tick...");
  }
}

void clearEeprom(){
   for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  Serial.println(F("eepromCleared"));
}


