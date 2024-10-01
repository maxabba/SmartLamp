#include "HomeSpanController.h"

SmartLamp::SmartLamp(LedController& controller) : Service::LightBulb(), ledController(controller) {
    power = new Characteristic::On();
    level = new Characteristic::Brightness(100);
    newBrightness = 10;
}

boolean SmartLamp::update() {
    boolean isOn = power->getNewVal();
    newBrightness = level->getNewVal();
    
    if (isOn) {
        ledController.startFadeTo(map(newBrightness, 0, 100, 0, (1 << ledController.getResolution()) - 1), 200);  // Fade to new brightness based on resolution
    }else{
        ledController.startFadeOut(200);
    }

    return true;
}

AutoModeSwitch::AutoModeSwitch(bool initialState) : Service::Switch() {
    power = new Characteristic::On(initialState);
    isOnAutoMode = initialState;
}

boolean AutoModeSwitch::update() {
    isOnAutoMode = power->getNewVal();
    return true;
}

void AutoModeSwitch::activateAutoMode() {
    isOnAutoMode = true;
    power->setVal(true);
}



void setupHomeSpan(LedController& ledController, SmartLamp*& smartLamp, AutoModeSwitch*& autoModeSwitch) {
    ledController.startSetupBlink(1000);
    homeSpan.setControlPin(0);
    homeSpan.setStatusPin(2);
    homeSpan.enableAutoStartAP();
    homeSpan.setSketchVersion("1.0.0");
    homeSpan.setApSSID("SmartLamp-Setup");
    homeSpan.setApPassword("12345678");
    homeSpan.setPairingCode("10025800");

    homeSpan.setStatusCallback(LedController::statusCallback);


    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
        smartLamp = new SmartLamp(ledController);
        autoModeSwitch = new AutoModeSwitch(true);  // Inizializza con la modalità auto attiva
        
    // Attiva la modalità auto
    autoModeSwitch->activateAutoMode();
    homeSpan.begin(Category::Lighting, "Smart Lamp");
    homeSpan.autoPoll(6200,1,1);
}



