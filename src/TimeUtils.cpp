#include "TimeUtils.h"
#include <HTTPClient.h>

TimeUtils* TimeUtils::instance = nullptr;  // Inizializzazione del puntatore statico
unsigned long TimeUtils::lastTimeCheck = 0;
const unsigned long TimeUtils::TIME_CHECK_INTERVAL = 60000;
uint8_t TimeUtils::currentTimeState = 0;
bool TimeUtils::timeSynced = false;

// Variabile per tracciare l'ultima chiamata della funzione calcolaAlbaTramonto
unsigned long TimeUtils::lastSunCalculation = 0;  
const unsigned long TimeUtils::SUN_CALCULATION_INTERVAL = 86400000;  // 24 ore in millisecondi
uint8_t TimeUtils::alba_locale = 0;  // Inizialmente impostato a -1 (valore non valido)
uint8_t TimeUtils::tramonto_locale = 0;  // Inizialmente impostato a -1 (valore non valido)


// Costruttore privato
TimeUtils::TimeUtils() {}

// Metodo per ottenere l'unica istanza della classe (singleton)
TimeUtils* TimeUtils::getInstance() {
    if (instance == nullptr) {
        instance = new TimeUtils();  // Crea l'istanza se non esiste
    }
    return instance;
}

// Funzione per ottenere il fuso orario automaticamente
String getTimeZone() {
    HTTPClient http;
    http.begin("http://ip-api.com/json/");
    int httpCode = http.GET();
    String timeZone = "UTC";  // Fuso orario predefinito

    if (httpCode == 200) {
        String payload = http.getString();
        int tzIndex = payload.indexOf("\"timezone\":\"") + 12;
        int tzEndIndex = payload.indexOf("\"", tzIndex);
        timeZone = payload.substring(tzIndex, tzEndIndex);
    }

    http.end();
    return timeZone;
}

// Sincronizzazione del tempo tramite NTP
void TimeUtils::syncTimeWithNTP(const char* timeServer) {
    String timeZone = getTimeZone();
    setenv("TZ", timeZone.c_str(), 1);  // Imposta il fuso orario ottenuto
    tzset();
    
    configTime(3600, 0, timeServer);  // 3600 è l'offset in secondi (+1 ora)
    
    unsigned long startMillis = millis();
    unsigned long timeout = 5000;  // Timeout di 5 secondi per la sincronizzazione

    struct tm timeinfo;
    bool syncComplete = false;

    while (!syncComplete && (millis() - startMillis < timeout)) {
        if (getLocalTime(&timeinfo)) {
            timeSynced = true;  // Sincronizzazione riuscita
            syncComplete = true;
        } else {
            delay(1);  // Ritardo leggero
        }
    }

    if (!syncComplete) {
        timeSynced = false;
        return;
    }

    // Attendi leggermente per garantire che l'orario locale sia pronto
    delay(100);  // Aggiungi un ritardo di 100 ms

    if (!getLocalTime(&timeinfo, 10)) {
        timeSynced = false;
    } else {

        // Chiama calcNightTime() subito dopo la sincronizzazione
        checkTime();  // Controlla il tempo per aggiornare lo stato
        calcNightTime();  // Calcola immediatamente lo stato giorno/notte
        calcSunTimes();  // Calcola l'alba e il tramonto subito dopo la sincronizzazione
    }
}

// Funzione che calcola se è giorno o notte (chiamata subito dopo la sincronizzazione)

void TimeUtils::calcNightTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) {
        return;  // Non fa nulla se il tempo non è disponibile
    }

    // Usa alba_locale e tramonto_locale se sono stati impostati (>= 0), altrimenti usa i valori predefiniti
    int8_t alba = (alba_locale >= 0) ? alba_locale : 8;  // Se alba_locale è valido, usalo, altrimenti usa 8
    int8_t tramonto = (tramonto_locale >= 0) ? tramonto_locale : 17;  // Se tramonto_locale è valido, usalo, altrimenti usa 17

    // Aggiorna lo stato giorno/notte
    if (timeinfo.tm_hour >= tramonto || timeinfo.tm_hour < alba) {
        currentTimeState = 1;  // Notte
    } else {
        currentTimeState = 2;  // Giorno
    }
}

// Funzione che calcola l'alba e il tramonto ogni 24 ore
void TimeUtils::calcSunTimes() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) {
        return;  // Non fa nulla se il tempo non è disponibile
    }

    int8_t timezoneOffset = 1;  // Aggiusta questo valore in base al tuo fuso orario
    bool daylightSaving = false;  // Regola se è in vigore l'ora legale

    calcolaAlbaTramonto(timeinfo, timezoneOffset, daylightSaving);
}

// Timer per chiamare calcolaAlbaTramonto ogni 24 ore
void TimeUtils::checkSunTimes() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastSunCalculation >= SUN_CALCULATION_INTERVAL) {
        lastSunCalculation = currentMillis;
        calcSunTimes();  // Calcola l'alba e il tramonto
    }
}

//define a timer function using millis() to check the time every minute
void TimeUtils::checkTime() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastTimeCheck >= TIME_CHECK_INTERVAL) {
        lastTimeCheck = currentMillis;
        struct tm timeinfo;
        calcNightTime();  // Calcola lo stato giorno/notte
    }

    // Controllo aggiuntivo per il calcolo dell'alba/tramonto
    checkSunTimes();  // Controlla se è il momento di aggiornare l'alba e il tramonto
}

// Ritorna lo stato corrente (giorno o notte)
uint8_t TimeUtils::isNightTime() {
    checkTime();  // Controlla il tempo per aggiornare lo stato
    return currentTimeState;
}

// Funzione per verificare se il tempo è sincronizzato
bool TimeUtils::isTimeSynced() {
    return timeSynced;  // Ritorna lo stato della sincronizzazione
}

uint16_t TimeUtils::calcolaGiornoDellAnno(const tm &data) {
    const uint8_t giorniPerMese[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    uint16_t giornoDellAnno = 0;
    for (uint8_t i = 0; i < data.tm_mon; ++i) {
        giornoDellAnno += giorniPerMese[i];
    }
    giornoDellAnno += data.tm_mday;  // Giorno del mese
    return giornoDellAnno;
}

// Funzione per calcolare l'orario di alba e tramonto approssimato, riceve una struttura tm come input
void TimeUtils::calcolaAlbaTramonto(const tm &data, int8_t fuso_orario, bool ora_legale) {
    // Calcolo del giorno dell'anno (1-365)
    uint16_t giornoDellAnno = calcolaGiornoDellAnno(data);
    
    // Stima della variazione stagionale: da 8 a 16 ore di luce, con media di 12 ore
    uint8_t mediaDurataGiorno = 12;  // ore di luce medie
    int8_t variazioneStagionale = 4 * cos((2 * PI * (giornoDellAnno - 80)) / 365);  // variazione in ore

    // Ore di luce totali (approssimato)
    uint8_t oreLuce = mediaDurataGiorno + variazioneStagionale;

    // Calcolo dell'alba e del tramonto in UTC, solo ore intere
    int8_t alba_utc = 12 - (oreLuce / 2);  // Ora dell'alba UTC
    int8_t tramonto_utc = 12 + (oreLuce / 2);  // Ora del tramonto UTC

    // Aggiungere il fuso orario e considerare l'ora legale
    uint8_t alba_locale = alba_utc + fuso_orario + (ora_legale ? 1 : 0);
    uint8_t tramonto_locale = tramonto_utc + fuso_orario + (ora_legale ? 1 : 0);

    // Correggere l'orario in caso superi 24 ore o sia negativo
    if (alba_locale >= 24) alba_locale -= 24;
    if (tramonto_locale >= 24) tramonto_locale -= 24;
    if (alba_locale < 0) alba_locale += 24;
    if (tramonto_locale < 0) tramonto_locale += 24;

    // Output su monitor seriale
    Serial.print(alba_locale);

    Serial.print(tramonto_locale);
}
