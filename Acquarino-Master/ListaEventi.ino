String stringaEventi() {
    orderByID();
    String str = "Lista Eventi : \n";
    for (int i = 0; i < eventsList.size(); i++) {
        Evento tmp = eventsList.at(i);
        str += tmp.nome_azione + " alle " + tmp.ora + ":" + tmp.minuti + ":" + tmp.secondi  +"  ID: " + tmp.ID_task+ "\n";
    }

    return str;
}

void modificaEvento(int eventID, int ora, int minuti, int secondi) {
    for (int i = 0; i < eventsList.size(); i++) {
        if (eventsList.at(i).ID_task == eventID) {
            Alarm.free(eventID);
            eventsList.at(i) = creaEventoStandard(eventsList.at(i).nome_azione, ora, minuti, secondi, eventsList.at(i).callbackFunction);
        }
    }

    
}

Evento creaEventoStandard(String nome , int ora, int minuti ,int secondi , OnTick_t callback){
  Evento tmp;
  tmp.nome_azione =nome;
  tmp.ora=ora;
  tmp.minuti = minuti;
  tmp.secondi = secondi;
  tmp.ID_task = Alarm.alarmRepeat(ora,minuti,secondi, callback);
  tmp.callbackFunction=callback;
  return tmp;
}

void addEvento(Evento e){
    eventsList.push_back(e);
    orderByID();

}

void orderByID(){
    for (int i = 0; i <eventsList.size()-1; i++){
        for (int j = i+1; j <eventsList.size(); j++){
            if(eventsList.at(i).ID_task>eventsList.at(j).ID_task){
                Evento tmp = eventsList.at(i);
                eventsList.at(i)=eventsList.at(j);
                eventsList.at(j)=tmp;
            }
        }
    }
    
}

void eliminaEvento(int id){
    for (int i = 0; i < eventsList.size(); i++) {
        if (eventsList.at(i).ID_task == id) {
            Alarm.free(id);
            eventsList.clear(i);
        }
    }

    scriviEventiFile();
}

void callCallbackFunction(OnTick_t function){
    function();
}

void scriviEventiFile(){
    File file = SPIFFS.open("/eventi.txt", "w");
 
    if (!file) {
        Serial.println("Error opening file for writing");
    return;
    }

    for(int i=0; i<eventsList.size(); i++){
        Evento e = eventsList.at(i);
        int c = file.println(eventoToString(e));
        if(c==0){
            Serial.println("Errore Scrittura !!");
            break;
        }

    }

    file.close();

}

void caricaListaEventi(){

    
        File file = SPIFFS.open("/eventi.txt", "r");
        if (!file  ) {
            client.publish("Acquarino/debug","File non Presente\n Carico eventi base");
            eventsList.push_back(creaEventoStandard("Accendi Luci",10,0,0,turnOnLights));
            eventsList.push_back(creaEventoStandard("Cibo",12,0,0,feedFish));
            eventsList.push_back(creaEventoStandard("Spegni Luci",18,0,0,turnOffLights));
            eventsList.push_back(creaEventoStandard("Accendi Areatore",19,0,0,turnOnAerator));
            eventsList.push_back(creaEventoStandard("Spegni Areatore",21,0,0,turnOffAerator));
            scriviEventiFile();
        }else{
            client.publish("Acquarino/debug","Carico Da File!");
            while (file.available()) {
                
               String line = file.readStringUntil('\n');
               
                //Serial.println(line);

                Evento tmp;
                tmp.nome_azione=getValue(line,',',0);
                String ora = getValue(line,',',1);
                tmp.ora = getValue(ora,':',0).toInt();
                tmp.minuti = getValue(ora,':',1).toInt();
                tmp.secondi = getValue(ora,':',2).toInt();
                int codiceFunzione =getValue(line,',',2).toInt();
                switch (codiceFunzione){
                case 0:
                    tmp.callbackFunction = turnOnLights;
                    break;

                case 1:
                    tmp.callbackFunction = turnOffLights;
                    break;

                case 2:
                    tmp.callbackFunction = turnOnAerator;
                    break;

                case 3:
                    tmp.callbackFunction = turnOffAerator;
                    break;
                case 4:
                    tmp.callbackFunction = feedFish;
                    break;
                default:
                    break;
                }

               
                addEvento(creaEventoStandard(tmp.nome_azione,tmp.ora,tmp.minuti,tmp.secondi,tmp.callbackFunction));
                

            }

            file.close();

        }

}


String eventoToString(Evento e){
    String str = "";
    str += e.nome_azione+",";
    str += String(e.ora)+":"+String(e.minuti)+":"+String(e.secondi)+",";
    
    if(e.callbackFunction == turnOnLights){
        str+="0";
    }else  if(e.callbackFunction == turnOffLights){
        str+="1";
    }else  if(e.callbackFunction == turnOnAerator){
        str+="2";
    }else  if(e.callbackFunction == turnOffAerator){
        str+="3";
    }else  if(e.callbackFunction == feedFish){
        str+="4";
    }

    return str;
}