#pragma once
#include "HomeSpan.h"
#include "LedController.h"
#include "MotionSensor.h"  // Include your custom MotionSensor class

class SmartLamp : public Service::LightBulb {
private:
    LedController& ledController;
    SpanCharacteristic *power;
    SpanCharacteristic *level;

public:
    SmartLamp(LedController& controller);
    boolean update() override;
};

class AutoModeSwitch : public Service::Switch {
private:
    SpanCharacteristic *power;
    bool isOnAutoMode;

public:
    AutoModeSwitch(bool initialState = false);
    boolean update() override;
    bool getIsOnAutoMode() const { return isOnAutoMode; }
    void activateAutoMode();
};