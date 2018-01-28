
void setRepeat()
{
  char *arg;

  arg = SCmd.next();
  if (arg != NULL)
  {
    configuration.alarmRepeat = atoi(arg);  // Converts a char string to an integer
    configuration.alarmRepeat = configuration.alarmRepeat * 60000 ;
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
    bool_en = atoi(arg);
    if (bool_en == 1) {
      configuration.wireless_en = true;
      Serial.println(F("Wireless Enabled"));
    } else if (bool_en == 0) {
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
    bool_en = atoi(arg);
    if (bool_en == 1) {
      configuration.debug = true;
      Serial.println(F("Debug Enabled"));
    } else if (bool_en == 0) {
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

  if ((tempHigh == NULL) or (tempLow == NULL)) {
    Serial.println(F("TEMP <HIGH> <LOW>"));
    return;
  }

  tempHigh_t = atoi(tempHigh);  // Converts a char string to an integer
  tempLow_t = atoi(tempLow);   // Converts a char string to an integer

  if ((tempHigh_t <= tempLow_t)) {
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

  if ((voltageHigh == NULL) or (voltageLow == NULL)) {
    Serial.println(F("VOLTAGE <HIGH> <LOW>"));
    return;
  }

  voltageHigh_t = atoi(voltageHigh);  // Converts a char string to an integer
  voltageLow_t = atoi(voltageLow);   // Converts a char string to an integer

  if ((voltageHigh_t <= voltageLow_t)) {
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

  if ((humidityHigh == NULL) or (humidityLow == NULL)) {
    Serial.println(F("HUMIDITY <HIGH> <LOW>"));
    return;
  }

  humidityHigh_t = atoi(humidityHigh);  // Converts a char string to an integer
  humidityLow_t = atoi(humidityLow);   // Converts a char string to an integer

  if ((humidityHigh_t <= humidityLow_t)) {
    Serial.println(F("Humidity rang error"));
    Serial.println(F("HUMIDITY <HIGH> <LOW>"));
    return;
  }

  configuration.humidityHigh = humidityHigh_t;
  configuration.humidityLow  = humidityLow_t;

  EEPROM.put(EEPROMStart, configuration);
  showConfig();
}

void setPhone()
{
  char arg[11];
  char phone[11];
  int  num = -1;

  num = atoi(SCmd.next());
  strlcpy(phone, SCmd.next(), 11);

  if ((num > 3) or (strlen(phone) == 0) or (num == -1)) {
    Serial.println(F("phone <number(1-3)> <phone>"));
    return;
  }

  num--;

  strlcpy(configuration.phone[num], phone, 11);

  EEPROM.put(EEPROMStart, configuration);

  Serial.println(F("Phone number saved"));
  showConfig();
}

void syncDate() {
  // Sync the date from the FONA
  // read the time
  // Time = "18/01/28,21:36:20+44"

  char buffer[23];
  fona.getTime(buffer, 23);  // make sure replybuffer is at least 23 bytes!
  
  char yr[3];
  char mh[3];
  char dy[3];
  char hr[3];
  char mn[3];
  char sd[3];
  
  sscanf(buffer,"%d[^/]/%d[^/]/%d[^/],%d[^:]:%d[^:]:%d[^:]", &yr, &dy, &mh, &hr, &mn, &sd);

  Serial.println(yr);
  Serial.println(mh);
  Serial.println(hr);
  Serial.println(mn);
  Serial.println(sd);
  
  setTime(hr, mn, sd, dy, mh, yr);
  if(timeStatus()!= timeSet) {
    Serial.println("Unable to sync with the RTC");
  }else{
    Serial.println("RTC has set the system time");
  }
}
