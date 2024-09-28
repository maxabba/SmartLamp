#pragma once
#include <Arduino.h>
#include "driver/ledc.h"
#include "HomeSpan.h"
#include "TimeUtils.h"

class LedController {
private:
    uint8_t pin;
    ledc_channel_t channel;
    ledc_timer_t timer;
    uint32_t freq;
    ledc_timer_bit_t resolution;
    uint32_t maxDuty;
    uint16_t currentBrightness;
    uint16_t targetBrightness;
    bool isFading;
    bool isBlinking;
    uint32_t blinkDuration;
    esp_timer_handle_t blinkTimer;
    static void IRAM_ATTR onFadeEnd(void* arg);
    static void IRAM_ATTR onBlinkTimer(void* arg);
    uint32_t calculateDuty(uint16_t brightness) const;

public:
    LedController(uint8_t pin, ledc_channel_t channel, ledc_timer_t timer, uint32_t freq, ledc_timer_bit_t resolution);
    void begin();
    void setBrightness(uint16_t brightness);
    void startFadeIn(uint32_t duration);
    void startFadeOut(uint32_t duration);
    void startFadeTo(uint16_t targetBrightness, uint32_t duration);
    bool isStillFading() const { return isFading; }
    uint16_t getCurrentBrightness() const { return currentBrightness; }
    ledc_timer_bit_t getResolution() const;
    void startSetupBlink(uint32_t blinkDur);
    void stopSetupBlink();

    static LedController* instance;

    static void statusCallback(HS_STATUS status){
        LedController& instance = getInstance();
        switch (status)
        {
        case HS_WIFI_NEEDED:
            instance.startSetupBlink(2000);
            break;
        case HS_WIFI_CONNECTING:
            instance.startSetupBlink(1000);
            break;
        case HS_PAIRED:
            instance.stopSetupBlink();        
            TimeUtils::getInstance()->syncTimeWithNTP("pool.ntp.org");
            break;
        case HS_PAIRING_NEEDED:
            instance.startSetupBlink(500);
            break;
        default:
            break;
        }
    }
    static LedController& getInstance() {
        return *instance;
    }

};