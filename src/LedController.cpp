#include "LedController.h"

LedController* LedController::instance = nullptr;

LedController::LedController(uint8_t pin, ledc_channel_t channel, ledc_timer_t timer, uint32_t freq, ledc_timer_bit_t resolution)
    : pin(pin), channel(channel), timer(timer), freq(freq), resolution(resolution),
      currentBrightness(0), targetBrightness(0), isFading(false), isBlinking(false) {
    instance = this;
    maxDuty = (1 << resolution) - 1;  // Calcolo del valore massimo del duty basato sulla risoluzione
}

void LedController::begin() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = resolution,
        .timer_num = timer,
        .freq_hz = freq,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = pin,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = channel,
        .timer_sel = timer,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

    ledc_fade_func_install(0);
}

uint32_t LedController::calculateDuty(uint16_t brightness) const {
    // Il valore massimo di luminosità dipende dalla risoluzione
    uint32_t maxBrightnessValue = (1 << resolution) - 1;
    return (brightness * maxDuty) / maxBrightnessValue;
}

void LedController::setBrightness(uint16_t brightness) {
    currentBrightness = brightness;
    uint32_t duty = calculateDuty(brightness);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel);
    isFading = false;
}

void LedController::startFadeIn(uint32_t duration) {
    startFadeTo((1 << resolution) - 1, duration);  // Imposta il massimo valore di luminosità
}

void LedController::startFadeOut(uint32_t duration) {
    startFadeTo(0, duration);  // Imposta la luminosità a 0
}

void LedController::startFadeTo(uint16_t newTargetBrightness, uint32_t duration) {
    targetBrightness = newTargetBrightness;
    uint32_t targetDuty = calculateDuty(targetBrightness);

    if (targetDuty > maxDuty) {
        targetDuty = maxDuty;
    }

    ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, channel, targetDuty, duration);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, channel, LEDC_FADE_NO_WAIT);

    isFading = true;

    esp_timer_create_args_t timerArgs = {
        .callback = &LedController::onFadeEnd,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "fade_timer"
    };
    esp_timer_handle_t fadeTimer;
    esp_timer_create(&timerArgs, &fadeTimer);
    esp_timer_start_once(fadeTimer, duration * 1000);
}

void IRAM_ATTR LedController::onFadeEnd(void* arg) {
    LedController* led = static_cast<LedController*>(arg);
    led->currentBrightness = led->targetBrightness;
    led->isFading = false;
}

void LedController::startSetupBlink(uint32_t blinkDur) {
    if (isBlinking) {
        stopSetupBlink();
    }

    blinkDuration = blinkDur;
    isBlinking = true;

    esp_timer_create_args_t timerArgs = {
        .callback = &LedController::onBlinkTimer,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "blink_timer"
    };
    esp_timer_create(&timerArgs, &blinkTimer);
    esp_timer_start_periodic(blinkTimer, blinkDuration * 1000);

    // Start with fade in
    startFadeTo(((1 << resolution) - 1)/128, blinkDuration / 2);  // Metti al massimo valore
}

void LedController::stopSetupBlink() {
    if (isBlinking) {
        esp_timer_stop(blinkTimer);
        esp_timer_delete(blinkTimer);
        isBlinking = false;
        setBrightness(0);
    }
}

void IRAM_ATTR LedController::onBlinkTimer(void* arg) {
    LedController* led = static_cast<LedController*>(arg);
    if (led->getCurrentBrightness() == 0) {
        led->startFadeTo(((1 << led->resolution) - 1)/128, led->blinkDuration / 2);  // Vai al massimo
    } else {
        led->startFadeTo(0, led->blinkDuration / 2);  // Vai a zero
    }
}

ledc_timer_bit_t LedController::getResolution() const {
    return resolution;
}
