#include "HomeSpan.h"
#include "LedController.h"
#include "HomeSpanController.h"
#include "MotionSensor.h"
#include "LampStateMachine.h"
#include "ESP32Time.h"
#include "time.h"

#define LED_PIN 18
#define LED_CHANNEL 0
#define FREQ 5000
#define RESOLUTION 12

LedController ledController(LED_PIN, LED_CHANNEL, FREQ, RESOLUTION);
AutoModeSwitch* autoModeSwitch;
SmartLamp* smartLamp;
MotionSensor motionSensor;
LampStateMachine lampStateMachine(ledController, motionSensor);
ESP32Time rtc(1);

bool isNightTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return false;
  }
  
  // Controlla se l'ora corrente è tra le 17:00 e le 08:00
  if (timeinfo.tm_hour >= 17 || timeinfo.tm_hour < 8) {
    return true;
  }
  return false;
}

void setup() {
    Serial.begin(115200);
    Serial.end();
    Serial2.begin(256000);  // Inizializza Serial2 per il sensore LD2410

    ledController.begin();
    motionSensor.begin();
    //lampStateMachine.begin();

    // Inizia l'effetto di blink con fade
    ledController.startSetupBlink(1000);  // 1 secondo per ciclo di blink completo
    
    homeSpan.setControlPin(0);
    homeSpan.setStatusPin(2);
    homeSpan.enableAutoStartAP();
    homeSpan.setSketchVersion("1.0.0");
    homeSpan.setApSSID("SmartLamp-Setup");
    homeSpan.setApPassword("12345678");
    homeSpan.setPairingCode("10025800");
    homeSpan.enableOTA();  
    homeSpan.enableWebLog(
      100,                           // maxEntries: numero massimo di voci da salvare
      "pool.ntp.org",                // timeServerURL: URL del server NTP per sincronizzare l'ora
      "CET-1CEST,M3.5.0,M10.5.0/3",  // timeZone: fuso orario in formato POSIX.1
      "weblog"                       // logURL: URL personalizzato per la pagina del Web Log
    );
    homeSpan.begin(Category::Lighting, "Smart Lamp");

    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
        smartLamp = new SmartLamp(ledController);
        autoModeSwitch = new AutoModeSwitch(true);  // Inizializza con la modalità auto attiva
        
    // Attiva la modalità auto
    autoModeSwitch->activateAutoMode();

    ledController.stopSetupBlink();
    Serial.println("Setup completato");
}

void loop() {
    homeSpan.poll();

    if(isNightTime()) {
        if (autoModeSwitch->getIsOnAutoMode()) {
            lampStateMachine.update(smartLamp->getNewBrightness());
        }
    }
}