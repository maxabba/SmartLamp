#include "LampStateMachine.h"

LampStateMachine::LampStateMachine(LedController& led, MotionSensor& motion)
    : currentState(LampState::OFF), ledController(led), motionSensor(motion), maxBrightness(100), stateTimer(nullptr) {
    initializeTransitionMatrix();
}

void LampStateMachine::begin() {
    esp_timer_create_args_t timerArgs = {
        .callback = &LampStateMachine::timerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "StateTimer"
    };
    esp_err_t err = esp_timer_create(&timerArgs, &stateTimer);
    if (err != ESP_OK) {
        Serial.printf("Failed to create timer: %s\n", esp_err_to_name(err));
        // Gestire l'errore in modo appropriato
    } else {
        Serial.println("Timer created successfully");
    }
}

void IRAM_ATTR LampStateMachine::timerCallback(void* arg) {
    Serial.println("Timer callback triggered");
    LampStateMachine* machine = static_cast<LampStateMachine*>(arg);
    machine->handleStateTransition(machine->currentState);
}

void LampStateMachine::setState(LampState newState) {
    if (currentState != newState) {
        LampState oldState = currentState;
        currentState = newState;
        Serial.printf("Transitioning from %d to %d\n", static_cast<int>(oldState), static_cast<int>(newState));
        transitionMatrix[static_cast<size_t>(oldState)][static_cast<size_t>(newState)](*this);
    }
}

void LampStateMachine::handleStateTransition(LampState newState) {
    setState(newState);
}

void LampStateMachine::update(uint8_t maxBrightness) {
    this->maxBrightness = maxBrightness;
    motionSensor.update();
    bool isMovement = motionSensor.isMovementDetected();
    bool isPresence = motionSensor.isPresenceDetected();

    switch (currentState) {
        case LampState::OFF:
            if (isMovement || isPresence) {
                setState(LampState::FULL_ON);
            }
            break;

        case LampState::FULL_ON:
            if (!isPresence) {
                setState(LampState::OFF);
            } else if (!isMovement) {
                setState(LampState::RELAXATION);
            }
            break;

        case LampState::RELAXATION:
            if (!isPresence) {
                setState(LampState::OFF);
            } else if (isMovement) {
                setState(LampState::FULL_ON);
            } else if (!isMovement && !isPresence) {
                setState(LampState::SLEEP);
            }
            break;

        case LampState::SLEEP:
            if (isMovement) {
                setState(LampState::SUDDEN_MOVEMENT);
            } else if (!isPresence) {
                setState(LampState::OFF);
            }
            break;

        case LampState::SUDDEN_MOVEMENT:
            if (!isPresence) {
                setState(LampState::OFF);
            } else if (!isMovement) {
                setState(LampState::RELAXATION);
            } else {
                setState(LampState::FULL_ON);
            }
            break;
    }
}

void LampStateMachine::resetTimer() {
    if (stateTimer != nullptr) {
        esp_timer_stop(stateTimer);
        esp_timer_delete(stateTimer);
        stateTimer = nullptr;
    }
    begin();  // Ricrea il timer
}

void LampStateMachine::startTimer(uint64_t timeout_us) {
    unsigned long now = millis();  // Get current timestamp

    if (stateTimer == nullptr) {
        Serial.printf("[%lu] Timer not initialized, recreating...\n", now);
        begin();
    }

    esp_err_t err = esp_timer_stop(stateTimer);  // Stop the timer if it's running
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        Serial.printf("[%lu] Failed to stop timer: %s\n", now, esp_err_to_name(err));
    }

    err = esp_timer_start_once(stateTimer, timeout_us);
    if (err != ESP_OK) {
        Serial.printf("[%lu] Failed to start timer: %s\n", now, esp_err_to_name(err));
        if (err == ESP_ERR_INVALID_STATE) {
            Serial.printf("[%lu] Timer in invalid state, resetting...\n", now);
            resetTimer();
            err = esp_timer_start_once(stateTimer, timeout_us);
            if (err != ESP_OK) {
                Serial.printf("[%lu] Failed to start timer after reset: %s\n", now, esp_err_to_name(err));
            }
        }
    } else {
        Serial.printf("[%lu] Timer started successfully for %llu us\n", now, timeout_us);
    }
}

void LampStateMachine::transitionToOff() {
    ledController.startFadeOut(2000);
    esp_timer_stop(stateTimer);
}

void LampStateMachine::transitionToFullOn() {
    uint16_t mappedBrightness = map(this->maxBrightness, 0, 100, 0, 4095);
    Serial.printf("Max brightness: %d, Mapped brightness: %d\n", this->maxBrightness, mappedBrightness);
    ledController.startFadeTo(mappedBrightness, 1000);
    startTimer(5 * 60 * 1000000); // 5 minutes
}

void LampStateMachine::transitionToRelaxation() {
    uint16_t mappedBrightness = map(this->maxBrightness, 0, 100, 0, 2048);
    Serial.printf("Max brightness: %d, Mapped brightness: %d\n", this->maxBrightness, mappedBrightness);
    ledController.startFadeTo(mappedBrightness, 2000);
    startTimer(15 * 60 * 1000000); // 15 minutes
}

void LampStateMachine::transitionToSleep() {
    uint16_t mappedBrightness = map(this->maxBrightness, 0, 100, 0, 512);
    Serial.printf("Max brightness: %d, Mapped brightness: %d\n", this->maxBrightness, mappedBrightness);
    ledController.startFadeTo(mappedBrightness, 3000);
    esp_timer_stop(stateTimer);
}

void LampStateMachine::transitionToSuddenMovement() {
    uint16_t mappedBrightness = map(this->maxBrightness, 0, 100, 0, 2048);
    Serial.printf("Max brightness: %d, Mapped brightness: %d\n", this->maxBrightness, mappedBrightness);
    ledController.startFadeTo(mappedBrightness, 1000);
    startTimer(30 * 1000000); // 30 seconds
}

void LampStateMachine::initializeTransitionMatrix() {
    for (auto& row : transitionMatrix) {
        row.fill([](LampStateMachine&) {});
    }

    transitionMatrix[static_cast<size_t>(LampState::OFF)][static_cast<size_t>(LampState::FULL_ON)] = 
        [this](LampStateMachine&) { transitionToFullOn(); };

    transitionMatrix[static_cast<size_t>(LampState::FULL_ON)][static_cast<size_t>(LampState::OFF)] = 
        [this](LampStateMachine&) { transitionToOff(); };
    transitionMatrix[static_cast<size_t>(LampState::FULL_ON)][static_cast<size_t>(LampState::RELAXATION)] = 
        [this](LampStateMachine&) { transitionToRelaxation(); };

    transitionMatrix[static_cast<size_t>(LampState::RELAXATION)][static_cast<size_t>(LampState::SLEEP)] = 
        [this](LampStateMachine&) { transitionToSleep(); };
    transitionMatrix[static_cast<size_t>(LampState::RELAXATION)][static_cast<size_t>(LampState::OFF)] = 
        [this](LampStateMachine&) { transitionToOff(); };

    transitionMatrix[static_cast<size_t>(LampState::SLEEP)][static_cast<size_t>(LampState::SUDDEN_MOVEMENT)] = 
        [this](LampStateMachine&) { transitionToSuddenMovement(); };
    transitionMatrix[static_cast<size_t>(LampState::SLEEP)][static_cast<size_t>(LampState::OFF)] = 
        [this](LampStateMachine&) { transitionToOff(); };

    transitionMatrix[static_cast<size_t>(LampState::SUDDEN_MOVEMENT)][static_cast<size_t>(LampState::RELAXATION)] = 
        [this](LampStateMachine&) { transitionToRelaxation(); };
}