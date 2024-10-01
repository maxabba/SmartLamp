#include "HomeSpan.h"
#include "LedController.h"
#include "HomeSpanController.h"
#include "MotionSensor.h"
#include "LampStateMachine.h"
#include "TimeUtils.h"
#include "Logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#define LED_PIN 18
#define LED_CHANNEL LEDC_CHANNEL_0
#define LED_TIMER LEDC_TIMER_0
#define LED_FREQ 25000
#define LED_RESOLUTION LEDC_TIMER_10_BIT

LedController ledController(LED_PIN, LED_CHANNEL, LED_TIMER, LED_FREQ, LED_RESOLUTION);
AutoModeSwitch* autoModeSwitch;
SmartLamp* smartLamp;
MotionSensor motionSensor;
EnergyThresholds energyThresholds;

//predefine void loopTask(void * parameter) {
void smartLampLoopTask(void * parameter) {
    TimeUtils* timeUtils = TimeUtils::getInstance();
    LOG_DEBUG("SmartLamp", "Task starting...");

    // Attendere che il tempo sia sincronizzato
    while (!timeUtils->isTimeSynced()) {
        LOG_DEBUG("SmartLamp", "Waiting for time synchronization...");
        vTaskDelay(pdMS_TO_TICKS(100));  // Aspetta 1 secondo prima di controllare di nuovo
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    // Una volta sincronizzato, continua con il loop principale
    for(;;) {
        uint8_t isNight = timeUtils->isNightTime();
        //motionSensor.update();
        if (isNight == 1) {  // Notte
            LampStateMachine& lamp = LampStateMachine::getInstance(ledController, motionSensor, energyThresholds);

            lamp.update(smartLamp->getNewBrightness(), autoModeSwitch->getIsOnAutoMode());
        }
        // Altre operazioni necessarie...

        vTaskDelay(pdMS_TO_TICKS(100));  // Delay di 1 secondo per non sovraccaricare i log
    }
}


void setup() {
    delay(10000);  // Attesa per il caricamento del monitor seriale
    Logger::begin(115200, LogLevel::INFO);  // Inizializza il logger
    LOG_INFO("Main", "Starting Smart Lamp application");


    //Serial.begin(115200);

    Serial2.begin(256000);  // Inizializza Serial2 per il sensore LD2410

    ledController.begin();

    motionSensor.begin();
    motionSensor.autoUpdate(2512, 2, 0);
    BaseType_t result = xTaskCreatePinnedToCore(
        smartLampLoopTask,  // Funzione da eseguire
        "LoopTask",         // Nome della task
        2512,               // Dimensione dello stack
        NULL,               // Parametri della task
        3,                  // Priorità
        NULL,               // Task handle (non necessario salvarlo)
        0                   // Core su cui eseguire la task (prova con il core 0)
    );

    if (result != pdPASS) {
        LOG_DEBUG("SmartLamp", "Failed to create task, error: %d", result);
    }
    setupHomeSpan(ledController, smartLamp, autoModeSwitch);

}




void loop() {
    //homeSpan.poll();

    vTaskDelete(NULL); // Opzionale: termina il task del loop principale

}