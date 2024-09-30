#include "MotionSensor.h"
#include "Logger.h"

MotionSensor::MotionSensor() 
    : presenceDetected(false), movementDetected(false), 
      movementDistance(0), stationaryDistance(0), energy(0) {
    LOG_INFO("MotionSensor", "Initialized");
}

void MotionSensor::begin() {
    sensor.begin(Serial2);  // Assumiamo che il sensore sia collegato a Serial2
    delay(500);  // Attesa per l'inizializzazione del sensore
    LOG_INFO("MotionSensor", "Sensor initialized on Serial2");
}

void MotionSensor::update() {
    if (sensor.presenceDetected() || sensor.movingTargetDetected() || sensor.isConnected()) {
        presenceDetected = sensor.presenceDetected();
        movementDetected = sensor.movingTargetDetected();
        movementDistance = sensor.movingTargetDistance();
        stationaryDistance = sensor.stationaryTargetDistance();
        energy = calculateEnergy();
        
        LOG_DEBUG("MotionSensor", "Updated - Presence: %d, Movement: %d, Energy: %.2f",
                  presenceDetected, movementDetected, energy);
    } else {
        LOG_WARNING("MotionSensor", "Sensor not connected");
    }
}

float MotionSensor::calculateEnergy() {
    float movingEnergy = 0;
    float stationaryEnergy = 0;

    if (movementDetected) {
        movingEnergy = sensor.movingTargetEnergy();
    }

    if (presenceDetected) {
        stationaryEnergy = sensor.stationaryTargetEnergy();
    }

    float combinedEnergy = movingEnergy * 1.5 + stationaryEnergy;
    float normalizedEnergy = constrain(combinedEnergy, 0, 100);

    LOG_DEBUG("MotionSensor", "Energy calculation - Moving: %.2f, Stationary: %.2f, Combined: %.2f, Normalized: %.2f",
              movingEnergy, stationaryEnergy, combinedEnergy, normalizedEnergy);

    return normalizedEnergy;
}