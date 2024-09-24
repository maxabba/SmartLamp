#include "LedController.h"

LedController* LedController::instance = nullptr;

LedController::LedController(uint8_t pin, uint8_t channel, uint32_t freq, uint8_t resolution)
    : pin(pin), channel(channel), freq(freq), resolution(resolution), 
      currentBrightness(0), targetBrightness(0), fadeStep(0), isFading(false) {
    instance = this;
}

void LedController::begin() {
    ledcSetup(channel, freq, resolution);
    ledcAttachPin(pin, channel);
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &LedController::onTimer, true);
    timerAlarmWrite(timer, 5000, true);
    timerAlarmEnable(timer);
}

void LedController::setBrightness(uint16_t brightness) {
    currentBrightness = brightness;
    ledcWrite(channel, brightness);
    isFading = false;
}

void LedController::startFadeIn(uint32_t duration) {
    startFadeTo(4095, duration);  // 4095 è il valore massimo per 12 bit
}

void LedController::startFadeOut(uint32_t duration) {
    startFadeTo(0, duration);
}

void LedController::startFadeTo(uint16_t newTargetBrightness, uint32_t duration) {
    targetBrightness = newTargetBrightness;
    int32_t brightnessChange = targetBrightness - currentBrightness;
    fadeStep = (brightnessChange > 0) ? 1 : -1;
    isFading = true;
    
    uint32_t steps = abs(brightnessChange);
    if (steps > 0) {
        uint32_t intervalMicros = (duration * 1000) / steps;
        timerAlarmWrite(timer, intervalMicros, true);
    } else {
        isFading = false;
    }
    
    timerAlarmEnable(timer);
}

void IRAM_ATTR LedController::onTimer() {
    if (instance) {
        instance->updateFade();
    }
}

void LedController::updateFade() {
    if (!isFading) return;

    currentBrightness += fadeStep;
    ledcWrite(channel, currentBrightness);

    if ((fadeStep > 0 && currentBrightness >= targetBrightness) ||
        (fadeStep < 0 && currentBrightness <= targetBrightness)) {
        currentBrightness = targetBrightness;
        ledcWrite(channel, currentBrightness);
        isFading = false;
        timerAlarmDisable(timer);
    }
}

void LedController::startSetupBlink(uint32_t blinkDur) {
    blinkDuration = blinkDur;
    blinkingSetup = true;

    esp_timer_create_args_t timerArgs = {
        .callback = &LedController::onBlinkTimer,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "blink_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &blinkTimer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(blinkTimer, blinkDuration * 600));  // Mezzo ciclo di blink
}

void LedController::stopSetupBlink() {
    if (blinkingSetup) {
        esp_timer_stop(blinkTimer);
        esp_timer_delete(blinkTimer);
        blinkingSetup = false;
        setBrightness(0);  // Assicurati che il LED sia spento alla fine
    }
}

void IRAM_ATTR LedController::onBlinkTimer(void* arg) {
    LedController* led = static_cast<LedController*>(arg);
    
    if (led->getCurrentBrightness() == 0) {
        led->startFadeTo(4095, led->blinkDuration / 2);
    } else if (led->getCurrentBrightness() == 4095) {
        led->startFadeTo(0, led->blinkDuration / 2);
    }
}