#include "HomeSpanController.h"

SmartLamp::SmartLamp(LedController& controller) : Service::LightBulb(), ledController(controller) {
    power = new Characteristic::On();
    level = new Characteristic::Brightness(100);
    Serial.println("Configuring SmartLamp...");

}

boolean SmartLamp::update() {
    boolean isOn = power->getNewVal();
    int newBrightness = level->getNewVal();
    this->newBrightness = newBrightness;
    if (isOn) {
        ledController.startFadeTo(map(newBrightness, 0, 100, 0, 4095), 200);  // Fade to new brightness over 1 second
    }else{
        ledController.startFadeOut(200);
    }

    Serial.print("Updating SmartLamp: ");
    Serial.print(isOn ? "ON" : "OFF");
    Serial.print(" fading to ");
    Serial.print(newBrightness);
    Serial.println("% brightness");

    return true;
}

AutoModeSwitch::AutoModeSwitch(bool initialState) : Service::Switch() {
    power = new Characteristic::On(initialState);
    isOnAutoMode = initialState;
    Serial.println("Configuring Auto Mode Switch...");
    Serial.print("Initial state: ");
    Serial.println(isOnAutoMode ? "ON" : "OFF");
}

boolean AutoModeSwitch::update() {
    isOnAutoMode = power->getNewVal();
    Serial.print("Auto Mode is now: ");
    Serial.println(isOnAutoMode ? "ON" : "OFF");
    return true;
}

void AutoModeSwitch::activateAutoMode() {
    isOnAutoMode = true;
    power->setVal(true);
    Serial.println("Auto Mode activated at startup");
}