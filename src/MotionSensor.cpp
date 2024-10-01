#include "MotionSensor.h"
#include "Logger.h"

MotionSensor::MotionSensor() 
    : presenceDetected(false), movementDetected(false), 
      movementDistance(0), stationaryDistance(0), energy(0) {
    LOG_INFO("MotionSensor", "Initialized");
}

void MotionSensor::begin() {
    sensor.begin(Serial2);  // Assumiamo che il sensore sia collegato a Serial2
    delay(500);  // Attesa per l'inizializzazione del sensore
    sensor.autoReadTask(1000, 1, 0);
    LOG_INFO("MotionSensor", "Sensor initialized on Serial2");
}

void MotionSensor::autoUpdate(uint32_t stack, uint32_t priority, uint32_t core) {
    xTaskCreatePinnedToCore(
        taskFunction,
        "MotionSensorTask",
        stack,
        this,
        priority,
        nullptr,
        core);
}

void MotionSensor::taskFunction(void* param) {
    MotionSensor* sensor = static_cast<MotionSensor*>(param);
    for (;;) {
        sensor->update();
        
        vTaskDelay(pdMS_TO_TICKS(10));  // Delay di 1 secondo per non sovraccaricare i log
    }
}


void MotionSensor::update() {
    
        presenceDetected = sensor.presenceDetected();
        movementDetected = sensor.movingTargetDetected();
        movementDistance = sensor.movingTargetDistance();
        stationaryDistance = sensor.stationaryTargetDistance();
        
        FrameData frame = sensor.getFrameData();
        if (frame.data != nullptr) {
            // Usa i dati del frame
            uploadFrame(frame.data, frame.length);
        } else {
            LOG_WARNING("MotionSensor", "Invalid frame data");
        }
        int stationaryEnergy = sensor.stationaryTargetEnergy();
        int movingEnergy = sensor.movingTargetEnergy();
        
        LOG_DEBUG("MotionSensor", "Presence: %d, Movement: %d, Movement Distance: %d, Stationary Distance: %d, Stationary Energy: %d, Moving Energy: %d",
                  presenceDetected, movementDetected, movementDistance, stationaryDistance, stationaryEnergy, movingEnergy);
}

float MotionSensor::calculateEnergy() {
    float movingEnergy = 0;
    float stationaryEnergy = 0;

    if (movementDetected) {
        movingEnergy = sensor.movingTargetEnergy();
    }

    if (presenceDetected) {
        stationaryEnergy = sensor.stationaryTargetEnergy();
    }

    float combinedEnergy = movingEnergy * 1.5 + stationaryEnergy;
    float normalizedEnergy = constrain(combinedEnergy, 0, 100);

    LOG_DEBUG("MotionSensor", "Energy calculation - Moving: %.2f, Stationary: %.2f, Combined: %.2f, Normalized: %.2f",
              movingEnergy, stationaryEnergy, combinedEnergy, normalizedEnergy);

    return normalizedEnergy;
}


String getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return "";
    }

    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &timeinfo);  // Formatta la data e ora
    return String(buffer);
}

void MotionSensor::uploadFrame(const uint8_t* frame, uint16_t frameLength) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://abbattista.cc/csv/save_data.php");

        // Ottieni l'ora corrente
        String timestamp = getFormattedTime();

        // Converti il frame e il timestamp in una stringa JSON
        String payload = "{ \"timestamp\": \"" + timestamp + "\", \"frame\": [";
        for (size_t i = 0; i < frameLength; i++) {
            payload += String(frame[i]);
            if (i < frameLength - 1) {
                payload += ", ";
            }
        }
        payload += "] }";

        // Invia il payload come JSON
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(payload);

        if (httpResponseCode > 0) {
            //Serial.println("Frame inviato con successo");
        } else {
            //Serial.println("Errore nell'invio del frame");
        }
        http.end();
    }
}

