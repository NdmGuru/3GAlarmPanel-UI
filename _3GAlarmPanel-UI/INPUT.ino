void readSHT11(){
  if(configuration.debug){
     Serial.println(F("DEBUG: Reading SHT Temp Sensor"));
  }

  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(DHT11_DATA, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.println(err);
    return;
  }
  if(configuration.debug){
    Serial.print(F("DEBUG: TEMP: "));
    Serial.println(temperature);
    Serial.print(F("DEBUG: HUMIDITY: "));
    Serial.println(humidity);
  }
  current.temp = temperature;
  current.humidity =  humidity; 
}

void readVoltage(int num){
  if(configuration.debug){
     Serial.print(F("DEBUG: Reading Voltage input "));
     Serial.println(num);
  }
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
    if(configuration.debug){
      Serial.print(F("DEBUG: Voltage In: "));
      Serial.println(current.voltageIn);
    }
  }else if (num == 1){
    current.voltageBatt = voltage * 11.132;
    if(configuration.debug){
      Serial.print(F("DEBUG: Voltage Batt: "));
      Serial.println(current.voltageBatt);
    }  
  }
  
  sample_count = 0;
  sum = 0;
}
