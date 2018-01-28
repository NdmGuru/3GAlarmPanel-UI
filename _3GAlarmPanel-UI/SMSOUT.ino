void buildAlertString(char *message_t){
  if(configuration.debug){
   Serial.println(F("DEBUG: Build Alert String"));
 }
 
  // These are 7 in length because we can have "-00.00\n"
  // This was a major cause of memory overwrite!
  char currentTemp[3] = "";
  char currentVoltage[6] = "";
  char currentHumidity[3] = "";
  
  dtostrf(current.temp, 2, 0, currentTemp); // NDM - No decimal on a sensor with +-2C accuracy
  dtostrf(current.voltageIn, 5, 2, currentVoltage);
  dtostrf(current.humidity, 2, 0, currentHumidity); // NDM As above


  if(configuration.debug){
    showFree();
  }

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

  if(configuration.debug){
    showFree();
    Serial.print(F("DEBUG: STRING="));
    Serial.println(message_t);
  }
  
  strcat(message_t,0);  
}

void queueAlerts(){
  Message message_out;
  char message_text[MAX_MSG_LEN] = "";
  
  buildAlertString(message_text);
  
  // For Each phone number we have, push a message to it
  for (int num = 0; num < 3; num++) {
    if(strlen(configuration.phone[num]) == 1){
      if(configuration.debug){
        Serial.println(F("DEBUG: Skipping blank phone number"));
      } // If Debug
    }else{
      // Queue the message
      if(configuration.debug){
        Serial.print(F("DEBUG: Queuing message to: "));
        Serial.println(configuration.phone[num]);
      }
      strcpy(message_out.text, message_text);
      strcpy(message_out.to, configuration.phone[num]);
      msgQueue.push(&message_out); 
    }// If Empty phone
  }// For Each phone
}

void sendAlerts(){
  if(configuration.debug){
    Serial.print(F("DEBUG: Sending "));
    Serial.print(msgQueue.nbRecs());
    Serial.println(F(" messages"));
  }

  Message this_message ;

  while(!msgQueue.isEmpty()){
    msgQueue.pop(&this_message);
    if(configuration.debug){
      showFree();
    }
    // For Each retry count needs to be added
    while(this_message.retry_cnt <= SMS_RETRY_CNT){
      if (configuration.wireless_en){
        if (!fona.sendSMS(this_message.to, this_message.text)) {
          Serial.print(F("Failed to send message to "));
          Serial.print(this_message.to);
          Serial.print(F(" attempt: "));
          Serial.print(this_message.retry_cnt);
          Serial.print(F(" of "));
          Serial.println(SMS_RETRY_CNT);
        } else {
          Serial.print(F("Message sent to "));
          Serial.println(this_message.to);
          break;
        }
      }else{
        Serial.print(F("Wireless Disabled, here's the message to "));
        Serial.print(this_message.to);
        Serial.print(F(": "));
        Serial.println(this_message.text);
        break;
      }
      this_message.retry_cnt++;    
    }
  }
}

