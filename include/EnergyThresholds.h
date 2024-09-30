#pragma once

#include <Arduino.h>
#include <CircularBuffer.hpp>
#include <Preferences.h>

class EnergyThresholds {
private:
    static const size_t BUFFER_SIZE = 1440; // Dati per un giorno con campionamento ogni minuto
    CircularBuffer<float, BUFFER_SIZE> energyBuffer;
    float th1, th2, th3;
    float alpha; // Fattore di smorzamento per EMA
    Preferences preferences;
    unsigned long lastSaveTime;
    static const unsigned long SAVE_INTERVAL = 3600000; // Salva ogni ora (in millisecondi)

    void updateThresholds();
    float calculateEMA(float currentValue, float previousValue);
    void loadThresholds();
    void saveThresholds();

public:
    EnergyThresholds(float initialTh1 = 50, float initialTh2 = 30, float initialTh3 = 10);
    void addEnergyReading(float energy);
    float getTh1() const { return th1; }
    float getTh2() const { return th2; }
    float getTh3() const { return th3; }
    void performLearning();
    void checkAndSave();
};