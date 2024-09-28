#include "LampStateMachine.h"

LampStateMachine::LampStateMachine(LedController& led, MotionSensor& motion)
    : currentState(LampState::OFF), ledController(led), motionSensor(motion), maxBrightness(100),
      stateStartTime(0), stateDuration(UINT32_MAX), debounceDelay(100) {
    initializeStateTransitionRules();
}

void LampStateMachine::initializeStateTransitionRules() {
    stateTransitionRules[static_cast<size_t>(LampState::OFF)] = [](bool isMovement, bool isPresence, bool) {
        return (isMovement || isPresence) ? LampState::FULL_ON : LampState::OFF;
    };

    stateTransitionRules[static_cast<size_t>(LampState::FULL_ON)] = [](bool isMovement, bool isPresence, bool stateTimedOut) {
        if ((!isPresence && !isMovement) && stateTimedOut) return LampState::OFF;
        if (!isMovement && stateTimedOut) return LampState::RELAXATION;
        return LampState::FULL_ON;
    };

    stateTransitionRules[static_cast<size_t>(LampState::RELAXATION)] = [](bool isMovement, bool isPresence, bool stateTimedOut) {
        if ((!isPresence && !isMovement ) && stateTimedOut) return LampState::OFF;
        if (isMovement && stateTimedOut ) return LampState::FULL_ON;
        if((isPresence && ! isMovement ) && stateTimedOut) return LampState::SLEEP;
        return LampState::RELAXATION;
    };

    stateTransitionRules[static_cast<size_t>(LampState::SLEEP)] = [](bool isMovement, bool isPresence, bool) {
        if ((!isPresence && !isMovement )) return LampState::OFF;
        if (isMovement) return LampState::SUDDEN_MOVEMENT;
        return LampState::SLEEP;
    };

    stateTransitionRules[static_cast<size_t>(LampState::SUDDEN_MOVEMENT)] = [](bool isMovement, bool isPresence, bool stateTimedOut) {
        if ((!isPresence && !isMovement ) && stateTimedOut) return LampState::OFF;
        if (isPresence && stateTimedOut) return LampState::SLEEP;
        if (isMovement) return LampState::SUDDEN_MOVEMENT;
        if (!isMovement && stateTimedOut) return LampState::RELAXATION;
        return LampState::SLEEP;
    };
}

void LampStateMachine::update(uint8_t maxBrightness, bool IsOnAutoMode) {

    if (IsOnAutoMode) {
        this->maxBrightness = maxBrightness;
        motionSensor.update();
        bool rawMovement = motionSensor.isMovementDetected();
        bool rawPresence = motionSensor.isPresenceDetected();
        
        bool stateTimedOut = millis() - stateStartTime > stateDuration;

        // Applica la regola di transizione in base allo stato attuale
        LampState newState = stateTransitionRules[static_cast<size_t>(currentState)](rawMovement, rawPresence, stateTimedOut);

        // Se lo stato cambia, aggiorna lo stato
        setState(newState);
    } else {
        stateDuration = UINT32_MAX;
        stateStartTime = millis();
        setState(LampState::OFF);
    }
}

void LampStateMachine::setState(LampState newState) {
    Serial.print("State: ");
    Serial.println(static_cast<int>(newState));
    if (currentState != newState) {
        LampState oldState = currentState;
        currentState = newState;

        static const std::array<void (LampStateMachine::*)(), static_cast<size_t>(LampState::STATE_COUNT)> transitionFunctions = {
            &LampStateMachine::transitionToOff,
            &LampStateMachine::transitionToFullOn,
            &LampStateMachine::transitionToRelaxation,
            &LampStateMachine::transitionToSleep,
            &LampStateMachine::transitionToSuddenMovement
        };

        if (static_cast<size_t>(newState) < transitionFunctions.size()) {
            (this->*transitionFunctions[static_cast<size_t>(newState)])();
        }

        stateStartTime = millis();
    }
}

void LampStateMachine::transitionToOff() {
    ledController.startFadeOut(2000);
    stateDuration = 0;
}

void LampStateMachine::transitionToFullOn() {
    uint16_t mappedBrightness = map(maxBrightness, 0, 100, 0, (1 << ledController.getResolution()) - 1);
    ledController.startFadeTo(mappedBrightness, 1000);
    stateDuration = 5 * 60 * 1000; // 5 minuti
}

void LampStateMachine::transitionToRelaxation() {
    uint16_t mappedBrightness = map(maxBrightness, 0, 100, 0, ((1 << ledController.getResolution()) - 1) / 2); // Metà del ciclo massimo
    ledController.startFadeTo(mappedBrightness, 2000);
    stateDuration = 15 * 60 * 1000; // 15 minuti
}

void LampStateMachine::transitionToSleep() {
    uint16_t mappedBrightness = map(maxBrightness, 0, 100, 0, ((1 << ledController.getResolution()) - 1) / 8); // 1/8 del ciclo massimo
    ledController.startFadeTo(mappedBrightness, 3000);
    stateDuration = UINT32_MAX; // Nessun timeout per lo stato SLEEP
}

void LampStateMachine::transitionToSuddenMovement() {
    uint16_t mappedBrightness = map(maxBrightness, 0, 100, 0, ((1 << ledController.getResolution()) - 1) / 2); // Metà del ciclo massimo
    ledController.startFadeTo(mappedBrightness, 1000);
    stateDuration = 30 * 1000; // 30 secondi
}