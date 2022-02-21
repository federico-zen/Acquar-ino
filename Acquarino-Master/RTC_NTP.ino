void RTCSetup(){
 Serial.println("setup RTC");
    if(!rtc.IsDateTimeValid()){

        Serial.println("Preparazione RTC tramite NTP");
        timeClient.begin();

        timeClient.update(); //prendo la data e l'ora
        
        //Devo risettare la data perche potrei avere la batteria scarica oppure mancante
        unsigned long epochTime = CE.toLocal(timeClient.getEpochTime()); //Utilizzo la librearia timezone per il fuso orario
        //unsigned long epochTime = timeClient.getEpochTime(); //senza fuso orario
        struct tm *ptm = gmtime ((time_t *)&epochTime);
        
        int monthDay = ptm->tm_mday;
        int currentMonth = ptm->tm_mon+1;
        int currentYear = ptm->tm_year+1900;
        int hour = ptm->tm_hour;
        int min = ptm->tm_min;
        int sec = ptm->tm_sec;

        char nowTime[30];
        snprintf_P(nowTime, 30,PSTR("%02u:%02u:%02u-%d-%d-%d"),hour,min,sec,monthDay,currentMonth,currentYear);
        
        Serial.println(String(nowTime));

        RtcDateTime now = RtcDateTime(currentYear,currentMonth,monthDay,hour,min,sec);
        rtc.SetDateTime(now); 
        
         
    }

    if(!rtc.GetIsRunning()){
        rtc.SetIsRunning(true);
    }

    if(!rtc.GetIsWriteProtected()){
        rtc.SetIsWriteProtected(false);
    }

}

void RTCReset(){
    timeClient.update(); //prendo la data e l'ora
    unsigned long epochTime = CE.toLocal(timeClient.getEpochTime());

        struct tm *ptm = gmtime ((time_t *)&epochTime);
        
        int monthDay = ptm->tm_mday;
        int currentMonth = ptm->tm_mon+1;
        int currentYear = ptm->tm_year+1900;
        int hour = ptm->tm_hour;
        int min = ptm->tm_min;
        int sec = ptm->tm_sec;
        setTime(hour,min,sec,monthDay,currentMonth,currentYear); 

        RtcDateTime now = RtcDateTime(currentYear,currentMonth,monthDay,hour,min,sec);
        rtc.SetDateTime(now); 
         

    if(!rtc.GetIsRunning()){
        rtc.SetIsRunning(true);
    }

    if(!rtc.GetIsWriteProtected()){
        rtc.SetIsWriteProtected(false);
    }
}