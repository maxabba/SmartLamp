#pragma once
#include <Arduino.h>
#include <ld2410.h>

class MotionSensor {
private:
    ld2410 sensor;
    bool presenceDetected;
    bool movementDetected;
    uint16_t movementDistance;
    uint16_t stationaryDistance;
    float energy;

    float calculateEnergy();

public:
    MotionSensor();
    void begin();
    void update();
    bool isPresenceDetected() const { return presenceDetected; }
    bool isMovementDetected() const { return movementDetected; }
    uint16_t getMovementDistance() const { return movementDistance; }
    uint16_t getStationaryDistance() const { return stationaryDistance; }
    float getEnergy() const { return energy; }
};