#include "costanti.h"
#include <Wire.h>
#include <Firmata.h>
#include <FirmataMarshaller.h>
#include <SoftwareSerial.h>
#include <TaskScheduler.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Timezone.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <QList.h>
#include <FS.h> 


//-------------------------------------
 
/**
 * Definizione dei pin lato Arduino 
 */
#define ARDUINO_TEMPERATURE_SERNSOR_PIN 0
#define ARDUINO_PH_SENSOR_PIN 1
#define ARDUINO_WATER_LEVEL_SENSOR_PIN 8 
#define ARDUINO_LIGHT_PIN 5 
#define ARDUINO_AIR_PUMP_PIN 7
#define ARDUINO_WATER_PUMP_PIN 6
#define ARDUINO_FEEDER 9


/**
 * Definizione dei pin lato ESP8266 
 */
//Software Serial PIN
#define SERIAL_RX 4
#define SERIAL_TX 0

//RTC PIN
#define DATA_RTC 2
#define CLK_RTC 14
#define RESET_RTC 12

//-------------------------------------

typedef struct  { //Struttura dati utilizzata per la gestione delle programmazioni
  String nome_azione;
  int  ora;
  int  minuti;
  int  secondi;
  int ID_task;
  OnTick_t callbackFunction;
}Evento;

//Vari orari delle attività
//Evento accensione_luci = {10,0,0};
//Evento spegnimento_luci = {18,0,0};
//Evento mangime_pesci = {12,0,0};
//Evento checkValori = {14,0,0};
//Evento promemoriaCambioAcqua = {10,0,0};


/**
 * Prototipi funzioni
 */

//Firmata
void analogWriteCallback(byte pin, int inValue);
void digitalWriteCallback(byte pin, int inValue);
void checkInputFromArduino();

//Alarm
void alarmSetup();

//RTC e NTP
void RTCSetup();
void RTCReset();

//WiFi e MQTT
void wiFiSetup();
void MQTTSetup();
void reconnect();
void MQTTCheck();
void callback(char * topic, byte * payload, unsigned int length);
void inviaDatiSensori();

//Telegram
void checkNewMessages();
void handleNewMessages(int numNewMessages);

//Gestione Acquario
void pinSetup();
void turnOnLights();
void turnOffLights();
void turnOnAerator();
void turnOffAerator();
void feedFish();
void waterChangeReminder();
void highValuesWarning();
void turnOnPump();
void turnOffPump();
void refillWater();
//-------------------------------------

/**
 * Variabili Sensori e stato Vasca
 */
float temperature=0;
float ph=0;
float voltage = 0;
int water_level=0;


long lastTimeWater=0;
long delayPump = 100; //ms 
long pumpTimeOut = 15000;
long lastTimePumpOn = 0;
boolean pumpError = false;

boolean luci = false;
boolean areatore = false;
boolean pompa =false;
boolean serviceMode = false; //indica se l'acquario è in manutenzione quindi il coperchio è aperto e di conseguenza il sensore di livello acqua potrebbe non leggere il valore corretto
boolean calibrationPH = false;

//Cicli del feeder ossia il numero di "porzioni"
int feeder_cycles = 1;

//FileSystem
boolean FS_OK;

//RTC e NTP
ThreeWire myWire(DATA_RTC,CLK_RTC,RESET_RTC); // IO, SCLK, CE
RtcDS1302<ThreeWire> rtc(myWire);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0);

// Central European Time 
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);

//Varibili Wifi e MQTT
WiFiClient espClient;
PubSubClient client(mqtt_server,1883, callback, espClient);

//Bot Telegram
WiFiClientSecure secure_client;
UniversalTelegramBot bot(BOTtoken, secure_client);

//Connessione Seriale Software che permette la comunicazione
SoftwareSerial mySerial(SERIAL_RX, SERIAL_TX); // RX, TX 

//marshaller per mandare comandi all'arduino UNO
firmata::FirmataMarshaller marshaller;

//Inizializzaione dello Scheduler per i Task
Scheduler taskScheduler;
Task CheckFirmataInput(100*TASK_MILLISECOND,TASK_FOREVER,checkInputFromArduino);
Task CheckMQTTInputs(200*TASK_MILLISECOND,TASK_FOREVER,MQTTCheck);
Task CheckNewTelegramMessages(1000*TASK_MILLISECOND,TASK_FOREVER,checkNewMessages);
Task InviaDatiMQTT(500*TASK_MILLISECOND,TASK_FOREVER,inviaDatiSensori);

QList<Evento> eventsList;



//-------------------------------------

void setup(){
  delay(5000); //ritado l'avvio cosi da permettere all'arduino di avviarsi completamente
  Serial.begin(9600);
  //Setup delle varie connessioni
  wiFiSetup();
  RTCSetup(); //All'avvio prendo l'ora dal server NTP e la salvo nel RTCs
  MQTTSetup();

  FS_OK = SPIFFS.begin(); //FileSystem
  if (!FS_OK) {
    client.publish("Acquarino/ctrl","Errore FS"); 
  }
   
  secure_client.setInsecure();// Serve per il bot di telegram
 
  mySerial.begin(57600);
 
  Firmata.begin(mySerial);
  marshaller.begin(mySerial);
  

  //Inizializzazione PIN Remoti su ARDUINO
  pinSetup();        

  Firmata.attach(ANALOG_MESSAGE, analogWriteCallback);
  Firmata.attach(DIGITAL_MESSAGE,digitalWriteCallback);


  //inizializzo il task scheduler e i vari Task
  taskScheduler.init();
  taskScheduler.addTask(CheckFirmataInput);
  CheckFirmataInput.enable();
  taskScheduler.addTask(CheckNewTelegramMessages);
  CheckNewTelegramMessages.enable();
  taskScheduler.addTask(CheckMQTTInputs);
  CheckMQTTInputs.enable();
  taskScheduler.addTask(InviaDatiMQTT);
  InviaDatiMQTT.enable();

  alarmSetup();

  
  //Inizializzazione Ota
  ArduinoOTA.onStart([]() {
    Serial.println("Start Ota");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd Ota");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

   
   ArduinoOTA.setHostname("Arcquar-ino");
   ArduinoOTA.setPassword((const char *)"Acquar-ino");
   ArduinoOTA.setPort(8266);
   Serial.println("OTA Setup");
   ArduinoOTA.begin();

  
   

}

void loop(){
 
  taskScheduler.execute();
  ArduinoOTA.handle();
  Alarm.delay(0);
}

void digitalWriteCallback(byte pin, int inValue){

  switch (pin){
    case ARDUINO_WATER_LEVEL_SENSOR_PIN:
      water_level=inValue;

      if(!serviceMode){
        refillWater(); //funzione che controlla se far partire la pompa oppure no
      }else{
        turnOffPump();
      }
      
    break;
  

  }

}

void analogWriteCallback(byte pin, int inValue){
  
  switch (pin){
    case ARDUINO_TEMPERATURE_SERNSOR_PIN: //dato della temperatura
      temperature = ((float)inValue / 100);
      break;

    case ARDUINO_PH_SENSOR_PIN: //PH
      ph = ((float)inValue)/100;
      
      break;
    
  }
  
}


void checkInputFromArduino(){
  while (Firmata.available()){
    Firmata.processInput();
  }
}

void alarmSetup(){
  RtcDateTime now = rtc.GetDateTime();
  setTime(now.Hour(),now.Minute(),now.Second(),now.Day(),now.Month(),now.Year());  

  //Da aggiungere il caricamento degli eventi in caso siano salvati su un file 
  
  caricaListaEventi();
  
  //Questi eventi sono non vanno modificati quindi non li aggiungo alla lista
  Alarm.alarmRepeat(14,0,0,highValuesWarning); //alle 14 controllo i valori
  Alarm.alarmRepeat(dowFriday,10,0,0,waterChangeReminder); //Ogni venerdi alle 10 ricordo di fare un cambio di acqua
  Alarm.alarmRepeat(dowSunday,3,0,0,RTCReset); //domenica resetto RTC cosi da aggiornare un eventuale ora legale/solare
    
  checkEventiOnRestart(); //Controllo con l orario e controllo gli eventi

  //Alarm.alarmRepeat(10,0,0, turnOnLights);  //alle 10 accedo le luci
  //Alarm.alarmRepeat(12,0,0,feedFish); //alle 12 nutro i pesci
  //Alarm.alarmRepeat(14,00,0,highValuesWarning); //alle 14 controllo i valori
  //Alarm.alarmRepeat(18,0,0,turnOffLights);  //alle 18 le spengo
  //Alarm.alarmRepeat(19,0,0,turnOnAerator); //Accendo areatore
  //Alarm.alarmRepeat(20,0,0,turnOffAerator); //Spengo areatore alle 20
  //Alarm.alarmRepeat(dowFriday,10,0,0,waterChangeReminder); //Ogni venerdi alle 10 ricordo di fare un cambio di acqua
  //Alarm.alarmRepeat(dowSunday,3,0,0,RTCReset); //domenica resetto RTC cosi da aggiornare un eventuale ora legale/solare

}


