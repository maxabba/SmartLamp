#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <time.h>
#include <WiFi.h>

struct AlbaTramontoParams {
    tm timeinfo;      // Struttura tm per data e ora
    int8_t fuso_orario;  // Fuso orario
    bool ora_legale;     // Indicatore per ora legale
};

class TimeUtils {
private:
    static TimeUtils* instance;  // Puntatore all'unica istanza della classe
    static unsigned long lastTimeCheck;
    static const unsigned long TIME_CHECK_INTERVAL;
    static uint8_t currentTimeState;
    static unsigned long lastSunCalculation;
    static const unsigned long SUN_CALCULATION_INTERVAL;


    static bool timeSynced;  // Variabile di controllo per sapere se il tempo è stato sincronizzato
    static uint8_t alba_locale;
    static uint8_t tramonto_locale;
    
    // Costruttore privato per prevenire istanziazione esterna
    TimeUtils();
    uint16_t calcolaGiornoDellAnno(const tm &data); 
    void checkSunTimes();  // Dichiarazione del metodo checkSunTimes
    void calcSunTimes();  // Dichiarazione del metodo calcSunTimes
public:
    // Metodo per ottenere l'unica istanza della classe (singleton)
    static TimeUtils* getInstance();

    // Sincronizzazione del tempo tramite NTP
    void syncTimeWithNTP(const char* timeServer);
    void checkTime();
    // Verifica se è notte
    uint8_t isNightTime();  // Add this line

    bool isTimeSynced();

    void calcNightTime();  // Declaration of calcNightTime method
    void calcolaAlbaTramonto(const tm &data, int8_t fuso_orario, bool ora_legale);
    uint8_t getAlba(){
        return alba_locale;
    }
    uint8_t getTramonto(){
        return tramonto_locale;
    }

};
