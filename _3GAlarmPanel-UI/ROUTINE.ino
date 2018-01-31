void updateStatus() {
  led(ORANGE);
  if (configuration.debug) {
    Serial.println(F("DEBUG: Update Status triggered"));
  }

  // Check the date is something sane, and update it if not. 
  if((timeStatus()!= timeSet) or (year() == 1970)) {
    if(configuration.debug){
       Serial.println(F("DEBUG: Time not set, setting..."));
    }
    syncDate();
  }
  
  // Assume we have no alarms or state change
  current.alarm = false;
  current.stateChange = false;

  // Copy current state to previous state
  previous = current;

  // Update the temp/humidity sensors
  readSHT11();
  readVoltage(0);
  readVoltage(1);

  // Update TEMP status
  if (current.temp >= configuration.tempHigh) {
    current.tempState = B00000010;
    current.alarm = true;
    if (configuration.debug) {
      Serial.println(F("DEBUG: TEMP High"));
    }
  } else if (current.temp <= configuration.tempLow) {
    current.tempState = B00000001;
    current.alarm = true;
    if (configuration.debug) {
      Serial.println(F("DEBUG: TEMP Low"));
    }
  } else {
    current.tempState = B00000000;
    if (configuration.debug) {
      Serial.println(F("DEBUG: Temp Normal"));
    }
  }

  // Update VOLTAGE status
  if (current.voltageIn >= configuration.voltageHigh) {
    current.voltageState = B00000010;
    current.alarm = true;
    if (configuration.debug) {
      Serial.println(F("DEBUG: VOLTAGE High"));
    }
  } else if (current.voltageIn <= configuration.voltageLow) {
    current.voltageState = B00000001;
    current.alarm = true;
    if (configuration.debug) {
      Serial.println(F("DEBUG: VOLTAGE Low"));
    }
  } else {
    current.voltageState = B00000000;
    if (configuration.debug) {
      Serial.println(F("DEBUG: Voltage Normal"));
    }
  }

  if (!current.alarm) {
    led(B000001);
    if (configuration.debug) {
      Serial.println(F("DEBUG: No Alarms Found"));
    }
  } else {
    led(B000010);
    if (configuration.debug) {
      Serial.println(F("DEBUG: Alarms Present"));
    }
  }

  if ((current.tempState != previous.tempState) or (current.voltageState != previous.voltageState)) {
    current.stateChange = true;
  } else if ((current.tempState == previous.tempState) and (current.voltageState == previous.voltageState)) {
    current.stateChange = false;
  }
  if (configuration.debug) {
    showFree();
  } else {
    char dateBuffer[12];
    sprintf(dateBuffer,"%02u:%02u:%02u ",hour(),minute(),second());

    Serial.print(F("Tick: "));
    Serial.println(dateBuffer);
  }
}

void clearEeprom() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  Serial.println(F("eepromCleared"));
}


