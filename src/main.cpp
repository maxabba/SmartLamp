#include "HomeSpan.h"
#include "LedController.h"
#include "HomeSpanController.h"
#include "MotionSensor.h"
#include "LampStateMachine.h"
#include <time.h>

#define LED_PIN 18
#define LED_CHANNEL 0
#define FREQ 5000
#define RESOLUTION 12

LedController ledController(LED_PIN, LED_CHANNEL, FREQ, RESOLUTION);
AutoModeSwitch* autoModeSwitch;
SmartLamp* smartLamp;
MotionSensor motionSensor;
LampStateMachine lampStateMachine(ledController, motionSensor);

bool stopBlink = false;

unsigned long lastTimeCheck = 0;
const unsigned long TIME_CHECK_INTERVAL = 60000;  // 60 secondi in millisecondi
uint8_t currentTimeState = 0;  // 0: non inizializzato, 1: notte, 2: giorno

uint8_t isNightTime() {
  unsigned long currentMillis = millis();
  
  // Controlla l'ora solo se è passato l'intervallo specificato
  if (currentMillis - lastTimeCheck >= TIME_CHECK_INTERVAL) {
    lastTimeCheck = currentMillis;
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return currentTimeState;  // Ritorna l'ultimo stato conosciuto in caso di errore
    }
    
    // Aggiorna lo stato del tempo
    if (timeinfo.tm_hour >= 17 || timeinfo.tm_hour < 8) {
      currentTimeState = 1;  // Notte
    } else {
      currentTimeState = 2;  // Giorno
    }

    // Ferma il blink solo quando otteniamo il primo time check valido
    if (stopBlink) {
      ledController.stopSetupBlink();
      stopBlink = false;
    }
  }
  
  return currentTimeState;
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(256000);  // Inizializza Serial2 per il sensore LD2410

    ledController.begin();
    motionSensor.begin();
    //lampStateMachine.begin();

    // Inizia l'effetto di blink con fade
    ledController.startSetupBlink(2000);  // 1 secondo per ciclo di blink completo
    
    homeSpan.setControlPin(0);
    homeSpan.setStatusPin(2);
    homeSpan.enableAutoStartAP();
    homeSpan.setSketchVersion("1.0.0");
    homeSpan.setApSSID("SmartLamp-Setup");
    homeSpan.setApPassword("12345678");
    homeSpan.setPairingCode("10025800");
    homeSpan.enableWebLog(
      100,
      "pool.ntp.org",
      "UTC+1"
    );
    homeSpan.begin(Category::Lighting, "Smart Lamp");

    // Creiamo le istanze qui, dopo homeSpan.begin()
    autoModeSwitch = new AutoModeSwitch(true);
    smartLamp = new SmartLamp(ledController);
    smartLamp->setAutoModeSwitch(autoModeSwitch); 
    new SpanAccessory();
        new Service::AccessoryInformation();
            new Characteristic::Identify();
        smartLamp;  // Questo aggiunge il servizio SmartLamp all'accessorio
        autoModeSwitch; 

    
    // Attiva la modalità auto
    autoModeSwitch->activateAutoMode();

    Serial.println("Setup completato");
}

void loop() {
    homeSpan.poll();

    uint8_t timeState = isNightTime();
    if (timeState == 1 || timeState == 0) {  // Notte o non inizializzato
        if (autoModeSwitch->getIsOnAutoMode()) {
            lampStateMachine.update(smartLamp->getNewBrightness());
        }
    }
}