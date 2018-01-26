void readSHT11(){
  if(configuration.debug){
     Serial.println(F("DEBUG: Reading SHT Temp Sensor"));
  }

  
  
  current.temp    = sht1x.readTemperatureC();
  current.humidity  = sht1x.readHumidity();
}

void readVoltage(){
    int sum = 0;                    // sum of samples taken
    unsigned char sample_count = 0;
    float voltage = 0.0;  
  
     while (sample_count < NUM_SAMPLES) {
        sum += analogRead(VOLTAGE_DATA1);
        sample_count++;
        delay(10);
    }

    voltage = ((float)sum / (float)NUM_SAMPLES * 5.015) / 1024.0;
    current.voltage = voltage * 11.132;
    
    sample_count = 0;
    sum = 0;
}

