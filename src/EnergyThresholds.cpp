#include "EnergyThresholds.h"
#include "Logger.h"

EnergyThresholds::EnergyThresholds(float initialTh1, float initialTh2, float initialTh3)
    : th1(initialTh1), th2(initialTh2), th3(initialTh3), alpha(0.1), lastSaveTime(0) {
    preferences.begin("energy_thresh", false);
    loadThresholds();
    LOG_INFO("EnergyThresholds", "Initialized with Th1=%.2f, Th2=%.2f, Th3=%.2f", th1, th2, th3);
}

void EnergyThresholds::addEnergyReading(float energy) {
    energyBuffer.push(energy);
    LOG_DEBUG("EnergyThresholds", "Added energy reading: %.2f", energy);
}

void EnergyThresholds::performLearning() {
    LOG_INFO("EnergyThresholds", "Performing learning");
    updateThresholds();
    checkAndSave();
}

void EnergyThresholds::updateThresholds() {
    float sumS = 0, sumRS = 0, sumSL = 0, sumSP = 0;
    int countS = 0, countRS = 0, countSL = 0, countSP = 0;

    for (size_t i = 0; i < energyBuffer.size(); i++) {
        float e = energyBuffer[i];
        if (e > th1) {
            sumS += e;
            countS++;
        } else if (e > th2) {
            sumRS += e;
            countRS++;
        } else if (e > th3) {
            sumSL += e;
            countSL++;
        } else {
            sumSP += e;
            countSP++;
        }
    }

    float avgS = countS > 0 ? sumS / countS : th1;
    float avgRS = countRS > 0 ? sumRS / countRS : th2;
    float avgSL = countSL > 0 ? sumSL / countSL : th3;
    float avgSP = countSP > 0 ? sumSP / countSP : th3 / 2;

    float oldTh1 = th1, oldTh2 = th2, oldTh3 = th3;
    th1 = calculateEMA(avgS - avgRS / 2, th1);
    th2 = calculateEMA((avgRS + avgSL) / 2, th2);
    th3 = calculateEMA(avgSL - avgSP / 2, th3);

    LOG_INFO("EnergyThresholds", "Updated thresholds: Th1: %.2f->%.2f, Th2: %.2f->%.2f, Th3: %.2f->%.2f",
             oldTh1, th1, oldTh2, th2, oldTh3, th3);
}

float EnergyThresholds::calculateEMA(float currentValue, float previousValue) {
    return alpha * currentValue + (1 - alpha) * previousValue;
}

void EnergyThresholds::loadThresholds() {
    th1 = preferences.getFloat("th1", th1);
    th2 = preferences.getFloat("th2", th2);
    th3 = preferences.getFloat("th3", th3);
    LOG_INFO("EnergyThresholds", "Loaded thresholds: Th1=%.2f, Th2=%.2f, Th3=%.2f", th1, th2, th3);
}

void EnergyThresholds::saveThresholds() {
    preferences.putFloat("th1", th1);
    preferences.putFloat("th2", th2);
    preferences.putFloat("th3", th3);
    lastSaveTime = millis();
    LOG_INFO("EnergyThresholds", "Saved thresholds: Th1=%.2f, Th2=%.2f, Th3=%.2f", th1, th2, th3);
}

void EnergyThresholds::checkAndSave() {
    unsigned long currentTime = millis();
    if (currentTime - lastSaveTime >= SAVE_INTERVAL) {
        saveThresholds();
    }
}