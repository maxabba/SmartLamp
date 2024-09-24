#include "MotionSensor.h"

MotionSensor::MotionSensor() : presenceDetected(false), movementDistance(0), stationaryDistance(0) {}

void MotionSensor::begin() {
    sensor.begin(Serial2);  // Assumiamo che il sensore sia collegato a Serial2
    delay(500);  // Attesa per l'inizializzazione del sensore
}

void MotionSensor::update() {
    if (sensor.isConnected()) {
        presenceDetected = sensor.presenceDetected();
        movementDetected = sensor.movingTargetDetected();
        movementDistance = sensor.movingTargetDistance();
        stationaryDistance = sensor.stationaryTargetDistance();
    }
}

