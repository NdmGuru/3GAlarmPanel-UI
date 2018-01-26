void readSHT11(){
   if(configuration.debug){
      Serial.println(F("DEBUG: Reading SHT Temp Sensor"));
   }
   
   unsigned long curMillis = millis();          // Get current time
   if (curMillis - trhMillis >= TRHSTEP) {      // Time for new measurements?
    measActive = true;
    measType = TEMP;
    if (error = sht.meas(TEMP, &rawData, NONBLOCK)) // Start temp measurement
      if(configuration.debug){
          Serial.println(F("DEBUG: Error reading SHT11 TEMP"));
      }
    trhMillis = curMillis;
  }
  if (measActive && (error = sht.measRdy())) { // Check measurement status
    if (error != S_Meas_Rdy)
      if(configuration.debug){
        Serial.println(F("DEBUG: SHT11 Not ready for read"));
      }
      if (measType == TEMP) {                    // Process temp or humi?
        measType = HUMI;
        current.temp = sht.calcTemp(rawData);
        if (error = sht.meas(HUMI, &rawData, NONBLOCK)) 
          if(configuration.debug){
            Serial.println(F("DEBUG: Error reading SHT11 HUMIDITY"));
          }
      } else {
        measActive = false;
        current.humidity = sht.calcHumi(rawData, temperature); // Convert raw sensor data    }
      }
  }
}

void readVoltage(int num){
  int read_pin = 0;
  int sum = 0;                    // sum of samples taken
  unsigned char sample_count = 0;
  float voltage = 0.0;  

  if(num == 0){
    read_pin = VOLTAGE_DATA0;
  }else if (num == 1){
    read_pin = VOLTAGE_DATA1;
  }else{
    return;
  }
  
   while (sample_count < NUM_SAMPLES) {
      sum += analogRead(read_pin);
      sample_count++;
      delay(10);
  }

  voltage = ((float)sum / (float)NUM_SAMPLES * 5.015) / 1024.0;
  
  if(num == 0){
    current.voltageIn = voltage * 11.132;
  }else if (num == 1){
    current.voltageBatt = voltage * 11.132;
  }
  
  sample_count = 0;
  sum = 0;
}
