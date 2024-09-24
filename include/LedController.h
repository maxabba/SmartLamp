#pragma once
#include <Arduino.h>

class LedController {
private:
    uint8_t pin;
    uint8_t channel;
    uint32_t freq;
    uint8_t resolution;
    uint16_t currentBrightness;  // Cambiato a uint16_t per supportare valori fino a 4095
    uint16_t targetBrightness;   // Cambiato a uint16_t
    int16_t fadeStep;            // Cambiato a int16_t per supportare passi più grandi
    bool isFading;
    hw_timer_t * timer;
    esp_timer_handle_t blinkTimer;
    uint32_t blinkDuration;
    bool blinkingSetup;

    static void IRAM_ATTR onTimer();
    void updateFade();

public:
    LedController(uint8_t pin, uint8_t channel, uint32_t freq, uint8_t resolution);
    void begin();
    void setBrightness(uint16_t brightness);  // Cambiato a uint16_t
    void startFadeIn(uint32_t duration);
    void startFadeOut(uint32_t duration);
    void startFadeTo(uint16_t targetBrightness, uint32_t duration);  // Cambiato a uint16_t
    bool isStillFading() const { return isFading; }
    uint16_t getCurrentBrightness() const { return currentBrightness; }  // Cambiato a uint16_t

    void startSetupBlink(uint32_t blinkDur);
    void stopSetupBlink();

    static LedController* instance;
    static void IRAM_ATTR onBlinkTimer(void* arg);
};