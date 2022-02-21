// file con Funzioni per MQTT e  gestione WiFi

/**
 * Funzione per l'esecuzione dei vari comandi dati via MQTT
 */
void executeCommands(String messaggio,char * topic){

  if(strcmp(topic,"Acquarino/lights")==0){
      if(messaggio.compareTo("on")==0){
      turnOnLights();
    }else if(messaggio.compareTo("off")==0){
      turnOffLights();
    }
  }else if(strcmp(topic,"Acquarino/air")==0){
      if(messaggio.compareTo("on")==0){
      turnOnAerator();
    }else if(messaggio.compareTo("off")==0){
      turnOffAerator();
    }
  }else if(strcmp(topic,"Acquarino/feed")==0){
    if(messaggio.compareTo("feed")==0){
      feedFish();
    }
  }else if(strcmp(topic,"Acquarino/feed/cicli")==0){
    feeder_cycles = messaggio.toInt();
  }else if(strcmp(topic , "Acquarino/ServiceMode")==0){
    if(messaggio.compareTo("on")==0){
      serviceMode=true;
    }else if(messaggio.compareTo("off")==0){
      serviceMode=false;
    }
  }else if(strcmp(topic,"Acquarino/phCalibration")==0){
    if(messaggio.compareTo("ENTERPH")==0){
      calibrationPH= true;
      client.publish("Acquarino/PhDebug", "calibrazione PH");
      marshaller.sendString("ENTERPH");
      

    }else if (messaggio.compareTo("EXITPH")==0){
      marshaller.sendString("EXITPH");
      client.publish("Acquarino/PhDebug", "fine Calibrazione");
      calibrationPH = false;

    }else if(messaggio.compareTo("CALPH")==0){
      marshaller.sendString("CALPH");
      client.publish("Acquarino/PhDebug", "Lettura Valore PH");
      

    }

  }
}

/**
 * funzione eseguita ogni volta che arriva un messaggio su un topic a cui viene fatto subscribe 
 */
void callback(char * topic, byte * payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] -> ");
    
    String messaggio ="";

    for (int i = 0; i < length; i++) {
      messaggio += (char)payload[i];
    }

    Serial.println(messaggio);

    executeCommands(messaggio,topic);
    
}

/**
 * Inizializza il Wifi
 */
void wiFiSetup(){
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connessione al Wi-fi");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.print("Connected to the WiFi network con ip -> ");
  Serial.println(WiFi.localIP());
  
}

/**
 * Invio i dati via MQTT 
 */
void inviaDatiSensori(){
  char msg [100];
  snprintf(msg, 100, "%.2f",ph );
  client.publish("Acquarino/PH", msg);

  snprintf(msg, 100, "%.2f",temperature );
  client.publish("Acquarino/Temperature", msg);


  if(water_level == HIGH){
    client.publish("Acquarino/LivelloAcqua", "Livello acqua nella Norma");
    
   
  }else{
    client.publish("Acquarino/LivelloAcqua", "Livello acqua Basso");
  }
  

  RtcDateTime now = rtc.GetDateTime();
  snprintf_P(msg, 100,PSTR("%02u:%02u"),now.Hour(),now.Minute());
  client.publish("Acquarino/Time", msg);

  
}

/**
 * Inizializzazione MQTT
 */
void MQTTSetup(){
   while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect(name)) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
     delay(2000);
      
    }
  }

  client.publish("Acquarino/ctrl","Acquarino Connesso a MQTT");
  client.subscribe("Acquarino/lights");
  client.subscribe("Acquarino/air");
  client.subscribe("Acquarino/feed");
  client.subscribe("Acquarino/feed/cicli");
  client.subscribe("Acquarino/phCalibration");
  client.subscribe("Acquarino/ServiceMode");
  
}
/**
 * Riconnessione al Wifi e al Server MQTT
 */
void reconnect() {

	if(WiFi.status() != WL_CONNECTED){
		
		Serial.print("Connecting to ");
		Serial.println(ssid);

		WiFi.reconnect();
		
		Serial.println("");
		Serial.println("WiFi connected");  
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());
    
	}

	if(WiFi.status() == WL_CONNECTED){
	
		MQTTSetup();
	}
}
/**
 * Controlla MQTT
 */
void MQTTCheck(){
  if (!client.connected()){
    reconnect();
  }
  client.loop();
}

