#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <time.h>
#include <WiFi.h>

class TimeUtils {
private:
    static TimeUtils* instance;  // Puntatore all'unica istanza della classe
    static unsigned long lastTimeCheck;
    static const unsigned long TIME_CHECK_INTERVAL;
    static uint8_t currentTimeState;
    static bool timeSynced;  // Variabile di controllo per sapere se il tempo è stato sincronizzato
    static portMUX_TYPE mux;  // Mutex per sincronizzare l'accesso alle variabili condivise

    // Costruttore privato per prevenire istanziazione esterna
    TimeUtils();

public:
    // Metodo per ottenere l'unica istanza della classe (singleton)
    static TimeUtils* getInstance();

    // Sincronizzazione del tempo tramite NTP
    void syncTimeWithNTP(const char* timeServer);
    
    // Verifica se è notte
    uint8_t isNightTime();  // Add this line

    bool isTimeSynced();

    void calcNightTime();  // Declaration of calcNightTime method


};
