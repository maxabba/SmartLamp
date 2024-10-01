#pragma once
#include <Arduino.h>
#include <ld2410.h>
#include <WiFi.h>
#include <HTTPClient.h>
class MotionSensor {
private:
    ld2410 sensor;
    bool presenceDetected;
    bool movementDetected;
    uint16_t movementDistance;
    uint16_t stationaryDistance;
    float energy;

    float calculateEnergy();
    void uploadFrame(const uint8_t* frame, uint16_t frameLength);
    static void taskFunction(void* param);
public:
    MotionSensor();
    void begin();
    void update();
    void autoUpdate(uint32_t stack, uint32_t priority, uint32_t core);
    bool isPresenceDetected() const { return presenceDetected; }
    bool isMovementDetected() const { return movementDetected; }
    uint16_t getMovementDistance() const { return movementDistance; }
    uint16_t getStationaryDistance() const { return stationaryDistance; }
    float getEnergy() const { return energy; }
};