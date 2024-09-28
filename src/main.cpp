#include "HomeSpan.h"
#include "LedController.h"
#include "HomeSpanController.h"
#include "MotionSensor.h"
#include "LampStateMachine.h"
#include "TimeUtils.h"

#define LED_PIN 18
#define LED_CHANNEL LEDC_CHANNEL_0
#define LED_TIMER LEDC_TIMER_0
#define LED_FREQ 25000
#define LED_RESOLUTION LEDC_TIMER_10_BIT

LedController ledController(LED_PIN, LED_CHANNEL, LED_TIMER, LED_FREQ, LED_RESOLUTION);
AutoModeSwitch* autoModeSwitch;
SmartLamp* smartLamp;
MotionSensor motionSensor;

//predefine void loopTask(void * parameter) {
void smartLampLoopTask(void * parameter) {
    TimeUtils* timeUtils = TimeUtils::getInstance();
    
    // Attendere che il tempo sia sincronizzato
    while (!timeUtils->isTimeSynced()) {
        vTaskDelay(1); // Attesa di 1 secondo prima di controllare nuovamente
    }
    vTaskDelay(5000);
    // Una volta sincronizzato, continua con il loop principale
    for(;;) {
        uint8_t isNight = timeUtils->isNightTime();
        if (isNight == 1) {  // Notte
            LampStateMachine& lamp = LampStateMachine::getInstance(ledController, motionSensor);
            lamp.update(smartLamp->getNewBrightness(), autoModeSwitch->getIsOnAutoMode());
        }
        
        // Altre operazioni necessarie...
        vTaskDelay(1); // Piccola pausa per evitare di sovraccaricare il core
    }
}

void setup() {
    Serial.begin(256000);

    Serial2.begin(256000);  // Inizializza Serial2 per il sensore LD2410

    ledController.begin();

    motionSensor.begin();

    xTaskCreatePinnedToCore(
        smartLampLoopTask,     // Funzione da eseguire
        "LoopTask",   // Nome della task
        4096,         // Stack size (adattare secondo necessità)
        NULL,         // Parametri della task
        1,            // Priorità
        NULL,         // Task handle (non necessario salvarlo)
        1             // Core su cui eseguire la task (0)
    );
    setupHomeSpan(ledController, smartLamp, autoModeSwitch);
}




void loop() {
    //homeSpan.poll();

    vTaskDelete(NULL); // Opzionale: termina il task del loop principale

}