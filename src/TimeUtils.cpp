#include "TimeUtils.h"
#include <HTTPClient.h>

TimeUtils* TimeUtils::instance = nullptr;  // Inizializzazione del puntatore statico
unsigned long TimeUtils::lastTimeCheck = 0;
const unsigned long TimeUtils::TIME_CHECK_INTERVAL = 60000;
uint8_t TimeUtils::currentTimeState = 0;
bool TimeUtils::timeSynced = false;

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
        calcNightTime();  // Calcola immediatamente lo stato giorno/notte
    }
}

// Funzione che calcola se è giorno o notte (chiamata subito dopo la sincronizzazione)
void TimeUtils::calcNightTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) {
        return;  // Non fa nulla se il tempo non è disponibile
    }

    // Aggiorna lo stato giorno/notte
    if (timeinfo.tm_hour >= 17 || timeinfo.tm_hour < 8) {
        currentTimeState = 1;  // Notte
    } else {
        currentTimeState = 2;  // Giorno
  }
}
// Ritorna lo stato corrente (giorno o notte)
uint8_t TimeUtils::isNightTime() {
    return currentTimeState;
}

// Funzione per verificare se il tempo è sincronizzato
bool TimeUtils::isTimeSynced() {
    return timeSynced;  // Ritorna lo stato della sincronizzazione
}
