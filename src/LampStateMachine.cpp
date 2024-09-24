#include "LampStateMachine.h"

LampStateMachine::LampStateMachine(LedController& led, MotionSensor& motion)
    : currentState(LampState::OFF), ledController(led), motionSensor(motion), maxBrightness(100),
      stateStartTime(0), stateDuration(UINT32_MAX) {
    initializeTransitionMatrix();
}

void LampStateMachine::setState(LampState newState) {
    if (currentState != newState) {
        LampState oldState = currentState;
        currentState = newState;
        Serial.printf("Transitioning from %d to %d\n", static_cast<int>(oldState), static_cast<int>(newState));
        transitionMatrix[static_cast<size_t>(oldState)][static_cast<size_t>(newState)](*this);
    }
}

void LampStateMachine::update(uint8_t maxBrightness) {
    this->maxBrightness = maxBrightness;
    motionSensor.update();
    bool rawMovement = motionSensor.isMovementDetected();
    bool rawPresence = motionSensor.isPresenceDetected();

    unsigned long currentTime = millis();

    // Debounce per il movimento
    if (rawMovement) {
        lastMovementTime = currentTime;
    }
    bool isMovement = (currentTime - lastMovementTime) <= debounceDelay;

    // Debounce per la presenza
    if (rawPresence) {
        lastPresenceTime = currentTime;
    }
    bool isPresence = (currentTime - lastPresenceTime) <= debounceDelay;

    bool stateTimedOut = (stateDuration > 0) && ((currentTime - stateStartTime) >= stateDuration);

    Serial.printf("Current State: %d, isMovement: %d, isPresence: %d, stateTimedOut: %d\n",
                  static_cast<int>(currentState), isMovement, isPresence, stateTimedOut);
    Serial.printf("currentTime: %lu, stateStartTime: %lu, stateDuration: %lu\n",
                  currentTime, stateStartTime, stateDuration);

    switch (currentState) {
        case LampState::OFF:
            if (isMovement || isPresence) {
                setState(LampState::FULL_ON);
            }
            break;

        case LampState::FULL_ON:
            if (!isPresence || stateTimedOut) {
                setState(LampState::OFF);
            } else if (!isMovement) {
                setState(LampState::RELAXATION);
            }
            break;

        case LampState::RELAXATION:
            if (!isPresence || stateTimedOut) {
                setState(LampState::OFF);
            } else if (isMovement) {
                setState(LampState::FULL_ON);
            }
            // Rimani in RELAXATION finché il timer non scade o finché non c'è movimento
            break;


        case LampState::SLEEP:
            if (!isPresence) {
                setState(LampState::OFF);
            } else if (isMovement) {
                setState(LampState::SUDDEN_MOVEMENT);
            }
            // Rimani in SLEEP finché non c'è movimento o presenza
            break;


        case LampState::SUDDEN_MOVEMENT:
            if (!isPresence || stateTimedOut) {
                setState(LampState::OFF);
            } else if (stateTimedOut) {
                setState(LampState::RELAXATION);
            } else if (!isMovement) {
                // Rimani in SUDDEN_MOVEMENT finché il timer non scade
            }
            // Se c'è movimento e presenza, rimani in SUDDEN_MOVEMENT
            break;
    }
}

void LampStateMachine::transitionToOff() {
    ledController.startFadeOut(2000);
    stateStartTime = millis();
    stateDuration = 0; // Nessun timeout per lo stato OFF
}


void LampStateMachine::transitionToFullOn() {
    uint16_t mappedBrightness = map(this->maxBrightness, 0, 100, 0, 4095);
    Serial.printf("Max brightness: %d, Mapped brightness: %d\n", this->maxBrightness, mappedBrightness);
    ledController.startFadeTo(mappedBrightness, 1000);
    stateStartTime = millis();
    stateDuration = 5 * 60 * 1000; // 5 minutes
}

void LampStateMachine::transitionToRelaxation() {
    uint16_t mappedBrightness = map(this->maxBrightness, 0, 100, 0, 2048);
    Serial.printf("Max brightness: %d, Mapped brightness: %d\n", this->maxBrightness, mappedBrightness);
    ledController.startFadeTo(mappedBrightness, 2000);
    stateStartTime = millis();
    stateDuration = 15 * 60 * 1000; // 15 minutes
}

void LampStateMachine::transitionToSleep() {
    uint16_t mappedBrightness = map(this->maxBrightness, 0, 100, 0, 512);
    Serial.printf("Max brightness: %d, Mapped brightness: %d\n", this->maxBrightness, mappedBrightness);
    ledController.startFadeTo(mappedBrightness, 3000);
    stateStartTime = millis();
    stateDuration = UINT32_MAX; // No timeout for SLEEP state
}

void LampStateMachine::transitionToSuddenMovement() {
    uint16_t mappedBrightness = map(this->maxBrightness, 0, 100, 0, 2048);
    Serial.printf("Max brightness: %d, Mapped brightness: %d\n", this->maxBrightness, mappedBrightness);
    ledController.startFadeTo(mappedBrightness, 1000);
    stateStartTime = millis();
    stateDuration = 30 * 1000; // 30 seconds
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

    transitionMatrix[static_cast<size_t>(LampState::RELAXATION)][static_cast<size_t>(LampState::FULL_ON)] = 
        [this](LampStateMachine&) { transitionToFullOn(); };

    // Da SUDDEN_MOVEMENT a FULL_ON
    transitionMatrix[static_cast<size_t>(LampState::SUDDEN_MOVEMENT)][static_cast<size_t>(LampState::FULL_ON)] = 
        [this](LampStateMachine&) { transitionToFullOn(); };

    // Da SUDDEN_MOVEMENT a OFF
    transitionMatrix[static_cast<size_t>(LampState::SUDDEN_MOVEMENT)][static_cast<size_t>(LampState::OFF)] = 
        [this](LampStateMachine&) { transitionToOff(); };
}