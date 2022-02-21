//File che conterrà le funzioni per l'invio e la ricezione dei messaggi dal BOT di telegram ('Acquarino')
void handleNewMessages(int numNewMessages) {
    String messaggio = "";

    for (int i = 0; i < numNewMessages; i++) {
        // Chat id of the requester
        String chat_id = String(bot.messages[i].chat_id);
        if (chat_id != CHAT_ID) {
            bot.sendMessage(chat_id, "Utente non autorizzato", "");
            continue;
        }

        // Print the received message
        String text = bot.messages[i].text;
        Serial.println(text);

        String from_name = bot.messages[i].from_name;

        if (text == "/start") {
            String welcome = "Benvenuto, " + from_name + ".\n";
            welcome += "Ecco la Lista dei comandi : \n\n";
            welcome += "/Stato Per visualizzare lo stato della vasca \n";
            welcome += "/Feed Per dare un pasto ai pesci  \n";
            welcome += "/LightsOn per accendere Le luci \n";
            welcome += "/LightsOff per spegnere Le luci  \n";
            welcome += "/AirOn per accendere L' Areatore  \n";
            welcome += "/AirOff per spegnere L' Areatore  \n";
            welcome += "/IP per ottenre l'indirizzo ip dell'acquario \n";
            welcome += "/Time per Ottenere l'ora corrente in acquario  \n";
            welcome += "/TimeReset per resettare RTC e prendere l'orario da NTP  \n";
            welcome += "/ServiceMode per mettere l'acquario in modalità manutenzione per effettuare operazioni all'interno della vasca \n";
            welcome += "/EventsList Lista degli eventi della giornata\n";
            welcome += "/EditEvent Per modificare Evento Sintassi: EditEvent+ID+xx:xx:xx\n";
            welcome += "/AddEvent Per Aggiungere un nuovo evento: AddEvent+nome+xx:xx:xx+codiceFunzione\n";
            welcome += "/DeleteEvent Per eliminare un evento : DeleteEvent+ID\n";
            welcome += "/ListFile per vedere la lista dei file \n";
            welcome += "/RemoveFile Per rimuovere il file eventi\n";
            
            bot.sendMessage(chat_id, welcome, "HTML");

        } else if (text == "/Stato") {
            messaggio = "Luci: ";
            if (luci) {
                messaggio += "Accese \n";
            } else {
                messaggio += "Spente \n";
            }

            messaggio += "Areatore: ";
            if (areatore) {
                messaggio += "Acceso \n";
            } else {
                messaggio += "Spento \n";
            }

            messaggio += "Pompa: ";
            if (pompa) {
                messaggio += "Accesa \n";
            } else {
                messaggio += "Spenta \n";
            }
            messaggio += "La temperatura  e' :";
            messaggio += String(temperature) + "\n";
            messaggio += "Il PH e' : ";
            messaggio += String(ph) + "\n";
            messaggio += "Livello Dell'acqua ";
            if (water_level == HIGH) {
                messaggio += "Nella norma \n";
            } else {
                messaggio += "Basso \n";
            }

            messaggio += "Errore Pompa "+String(pumpError)+"\n";

            bot.sendMessage(chat_id, messaggio, "");

        } else if (text == "/Feed") {
            feedFish();
            messaggio = "Erogazione cibo in corso ....";
            bot.sendMessage(chat_id, messaggio, "");

        } else if (text == "/LightsOn") {
            turnOnLights();
            messaggio = "Accensione Luci in corso ....";
            bot.sendMessage(chat_id, messaggio, "");
        } else if (text == "/LightsOff") {
            turnOffLights();
            messaggio = "Spegnimento Luci in corso ....";
            bot.sendMessage(chat_id, messaggio, "");
        } else if (text == "/AirOff") {
            turnOffAerator();
            messaggio = "Spegnimento Areatore in corso ....";
            bot.sendMessage(chat_id, messaggio, "");
        } else if (text == "/AirOn") {
            turnOnAerator();
            messaggio = "Accensione Areatore in corso ....";
            bot.sendMessage(chat_id, messaggio, "");
        } else if (text == "/TimeReset") {
            messaggio = "Resetto RTC ....";
            bot.sendMessage(chat_id, messaggio, "");
            RTCReset();
        } else if (text == "/Time") {
            RtcDateTime now = rtc.GetDateTime();
            char msg[10];
            snprintf_P(msg, 10, PSTR("%02u:%02u"), now.Hour(), now.Minute());
            messaggio = String(msg);
            bot.sendMessage(chat_id, messaggio, "");
        } else if (text == "/ServiceMode") {
            if (serviceMode) {
                messaggio = "Disattivazione della modalità manuntenzione \n";
                serviceMode = false;
            } else {
                messaggio = "Attivazione della modalità manuntenzione \n";
                serviceMode = true;
            }
            bot.sendMessage(chat_id, messaggio, "");
        } else if (text == "/EventsList") {
            messaggio = stringaEventi();
            bot.sendMessage(chat_id, messaggio, "");
        } else if (text.substring(0,10) == "/EditEvent") {
            String toParse =text.substring(11) ;
            int id = getValue(toParse,'+',0).toInt();
            String ora=getValue(toParse,'+',1);

            modificaEvento(id,getValue(ora,':',0).toInt(),getValue(ora,':',1).toInt(),getValue(ora,':',2).toInt());
            scriviEventiFile();
            bot.sendMessage(chat_id, "Modificato Evento ID "+ String(id), "");
        }else if (text.substring(0,9) == "/AddEvent"){
            String toParse =text.substring(10);
            //AddEvent+nome+xx:xx:xx+codiceFunzione
            String ora=getValue(toParse,'+',1);
            int codiceFunzione=getValue(toParse,'+',2).toInt();

            Evento tmp;
            tmp.nome_azione=getValue(toParse,'+',0);
            tmp.ora = getValue(ora,':',0).toInt();
            tmp.minuti = getValue(ora,':',1).toInt();
            tmp.secondi = getValue(ora,':',2).toInt();
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
            scriviEventiFile();
            bot.sendMessage(chat_id, "Aggiunto", "");

        }else if (text == "/IP"){
            messaggio = String("IP : ")+ String(WiFi.localIP().toString()) ;
            bot.sendMessage(chat_id, messaggio, "");
        }else if(text.substring(0,12) == "/DeleteEvent"){
            int id =text.substring(13).toInt();
            eliminaEvento(id);
            scriviEventiFile();
        }else if(text == "/ListFile"){
            messaggio="";
            Dir dir = SPIFFS.openDir ("");
            while (dir.next ()) {
                messaggio +=" " + dir.fileName() +" "+dir.fileSize()+"\n";
            }
            bot.sendMessage(chat_id, messaggio, "");
        }else if(text == "/RemoveFile"){
            SPIFFS.remove("/eventi.txt");
        }else if(text == "/resetPump"){
            pumpError = false;
            bot.sendMessage(chat_id, "Errore Pompa Resettato", "");
        }
    }
}

void checkNewMessages() {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
}

void waterChangeReminder() {
    //Avviso sul bot Telegram che l'acqua è da cambiare
    String message = "&#x26A0; Attenzione &#x26A0; \n Cambio D'acqua Necessario";
    bot.sendMessage(CHAT_ID, message, "HTML");
}

void highValuesWarning() {
    if (ph > 7.5 || temperature >= 30 || temperature < 22 || ph < 6) {
        String message = "&#x26A0; Attenzione &#x26A0; \n";
        if (ph > 7.5) {
            message += "Il PH e' Alto ,oltre il 7.5!!";
        }else if(ph <6){
            message += "Il PH e' basso ,sotto il 6!!";
        }

        if (temperature >= 30) {
            message += "\nTemperatura oltre i 30 gradi!!";
        } else if (temperature < 22) {
            message += "\nTemperatura sotto i 22 gradi!!";
        }

        bot.sendMessage(CHAT_ID, message, "HTML");
    }
}

void pumpErrorMessage(){
    String message = "&#x26A0; Attenzione &#x26A0; \nLa pompa sta andando a vuoto \n Riempire la tanica o controllare il tubo , e digitare /resetPump\n";
    bot.sendMessage(CHAT_ID,message,"HTML");
}

//Funzione per lo split delle stringhe
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}