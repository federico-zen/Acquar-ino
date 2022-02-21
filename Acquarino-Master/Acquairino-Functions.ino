

void pinSetup(){
    marshaller.sendPinMode(ARDUINO_TEMPERATURE_SERNSOR_PIN, INPUT); 
    marshaller.sendPinMode(ARDUINO_PH_SENSOR_PIN, INPUT);   
    marshaller.sendPinMode(ARDUINO_WATER_LEVEL_SENSOR_PIN, INPUT);
    marshaller.sendPinMode(ARDUINO_LIGHT_PIN,OUTPUT);
    marshaller.sendPinMode(ARDUINO_AIR_PUMP_PIN,OUTPUT);
    marshaller.sendPinMode(ARDUINO_WATER_PUMP_PIN,OUTPUT);
    marshaller.sendPinMode(ARDUINO_FEEDER,PIN_MODE_PWM);

}


void turnOnLights(){
    marshaller.sendDigital(ARDUINO_LIGHT_PIN,HIGH);
    Serial.println("Turning on Lights");
    client.publish("Acquarino/ctrl","Luci Accese");
    luci =true;
}

void turnOffLights(){
    marshaller.sendDigital(ARDUINO_LIGHT_PIN,LOW);
    Serial.println("Turning off Lights");
    client.publish("Acquarino/ctrl","Luci Spente");
    luci =false;
}

void turnOnAerator(){
    marshaller.sendDigital(ARDUINO_AIR_PUMP_PIN,HIGH);
    Serial.println("Turning on Areator");
    client.publish("Acquarino/ctrl","Areatore Acceso");
    areatore =true;
}
void turnOffAerator(){
    marshaller.sendDigital(ARDUINO_AIR_PUMP_PIN,LOW);
    client.publish("Acquarino/ctrl","Areatore Spento");
    areatore=false;
}

void turnOnPump(){
    marshaller.sendDigital(ARDUINO_WATER_PUMP_PIN,HIGH);
    client.publish("Acquarino/ctrl","Pompa Accesa");
    pompa = true;
}

void turnOffPump(){
    marshaller.sendDigital(ARDUINO_WATER_PUMP_PIN,LOW);
    client.publish("Acquarino/ctrl","Pompa Spenta");
    pompa = false;
}

void feedFish(){

    for(int i = 0; i<feeder_cycles;i++){
        marshaller.sendAnalog(ARDUINO_FEEDER,0); //Lo uso come se fosse un trigger
        Alarm.delay(1000);
    }
    client.publish("Acquarino/ctrl","Somministro Cibo");
}


void refillWater(){
    if(!pumpError){
        if((millis()-lastTimeWater) >delayPump){
            lastTimeWater = millis();
            if(pompa == true && water_level == HIGH){
                turnOffPump();


            }else if(pompa == false && water_level == LOW){
                turnOnPump();
                lastTimePumpOn = millis();
                
            }
        }
    }

    if(pompa == true && (millis() - lastTimePumpOn)>=pumpTimeOut && pumpError==false){
        pumpError = true;
        turnOffPump();
        pumpErrorMessage();

    }
    

    
}

//Funzione che controlla gli eventi e modifica lo stato degli attuatori in caso in cui non siano nello stato corretto.
void checkEventiOnRestart(){
    for (int i = 0; i < eventsList.size(); i++) {
        RtcDateTime now = rtc.GetDateTime();
        Evento e = eventsList.at(i);
        if(now.Hour() >= e.ora && now.Minute() >= e.minuti && now.Second() >= e.secondi){
            if(e.nome_azione != "Cibo"){ //Il cibo lo faccio dare una volta sola , anche se si riavvia
                callCallbackFunction(e.callbackFunction);
            }
        }     
    }
}


