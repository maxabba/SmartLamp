#include "LampStateMachine.h"

#include "Logger.h"

LampStateMachine* LampStateMachine::instance = nullptr;

LampStateMachine& LampStateMachine::getInstance(LedController& led, MotionSensor& motion, EnergyThresholds& thresholds) {
    if (instance == nullptr) {
        instance = new LampStateMachine(led, motion, thresholds);
    }
    return *instance;
}

LampStateMachine::LampStateMachine(LedController& led, MotionSensor& motion, EnergyThresholds& thresholds)
    : currentState(LampState::NP), ledController(led), motionSensor(motion), energyThresholds(thresholds),
      maxBrightness(100), stateStartTime(0), stateDuration(UINT32_MAX), debounceDelay(100) {
    initializeStateTransitionRules();
    LOG_INFO("LampStateMachine", "Initialized");
}

void LampStateMachine::initializeStateTransitionRules() {
    stateTransitionRules[static_cast<size_t>(LampState::NP)] = [this](float energy, bool isMovement, bool isPresence, bool) {
        if (isPresence && !isMovement) return LampState::RS;
        if (isMovement) return LampState::S;
        return LampState::NP;
    };

    stateTransitionRules[static_cast<size_t>(LampState::S)] = [this](float energy, bool isMovement, bool isPresence, bool stateTimedOut) {
        if (!isPresence && !isMovement) return LampState::NP;
        if (!isMovement && energy <= energyThresholds.getTh1() && stateTimedOut) return LampState::RS;
        return LampState::S;
    };

    stateTransitionRules[static_cast<size_t>(LampState::RS)] = [this](float energy, bool isMovement, bool isPresence, bool stateTimedOut) {
        if (!isPresence && !isMovement) return LampState::NP;
        if (isMovement && energy > energyThresholds.getTh1()) return LampState::S;
        if (energy <= energyThresholds.getTh2() && stateTimedOut) return LampState::SL;
        return LampState::RS;
    };

    stateTransitionRules[static_cast<size_t>(LampState::SL)] = [this](float energy, bool isMovement, bool isPresence, bool) {
        if (!isPresence && !isMovement) return LampState::NP;
        if (isMovement && energy > energyThresholds.getTh2()) return LampState::RS;
        if (energy <= energyThresholds.getTh3()) return LampState::SP;
        return LampState::SL;
    };

    stateTransitionRules[static_cast<size_t>(LampState::SP)] = [this](float energy, bool isMovement, bool isPresence, bool) {
        if (!isPresence && !isMovement) return LampState::NP;
        if (isMovement && energy > energyThresholds.getTh3()) return LampState::R;
        return LampState::SP;
    };

    stateTransitionRules[static_cast<size_t>(LampState::R)] = [this](float energy, bool isMovement, bool isPresence, bool stateTimedOut) {
        if (!isPresence && !isMovement) return LampState::NP;
        if (isMovement && energy > energyThresholds.getTh1()) return LampState::S;
        if (!isMovement && energy <= energyThresholds.getTh2() && stateTimedOut) return LampState::RS;
        return LampState::R;
    };

    stateTransitionRules[static_cast<size_t>(LampState::A)] = [this](float, bool, bool, bool) {
        // Logica per uscire dallo stato di anomalia
        return LampState::NP;
    };
}

void LampStateMachine::update(uint8_t maxBrightness, bool IsOnAutoMode) {
    if (IsOnAutoMode) {
        this->maxBrightness = maxBrightness;
        
        bool isMovement = motionSensor.isMovementDetected();
        bool isPresence = motionSensor.isPresenceDetected();
        float energy = motionSensor.getEnergy();
        
        //energyThresholds.addEnergyReading(energy);
        
        bool stateTimedOut = millis() - stateStartTime > stateDuration;

        LampState newState = stateTransitionRules[static_cast<size_t>(currentState)](energy, isMovement, isPresence, stateTimedOut);

        setState(newState);

        static uint32_t lastLearningTime = 0;
        if (millis() - lastLearningTime > 3600000) {
            LOG_INFO("LampStateMachine", "Performing hourly learning");
            //energyThresholds.performLearning();
            lastLearningTime = millis();
        }
    } else {
        stateDuration = UINT32_MAX;
        stateStartTime = millis();
        setState(LampState::NP);
    }
}

void LampStateMachine::setState(LampState newState) {
    if (currentState != newState) {
        LOG_INFO("LampStateMachine", "State transition: %s -> %s", 
                 getStateName(currentState), getStateName(newState));
        currentState = newState;
        stateStartTime = millis();

        switch (newState) {
            case LampState::NP: transitionToNP(); break;
            case LampState::S: transitionToS(); break;
            case LampState::RS: transitionToRS(); break;
            case LampState::SL: transitionToSL(); break;
            case LampState::SP: transitionToSP(); break;
            case LampState::R: transitionToR(); break;
            case LampState::A: transitionToA(); break;
            default: break;
        }
    }
}

void LampStateMachine::transitionToNP() {
    ledController.startFadeOut(2000);
    stateDuration = 0;
}

void LampStateMachine::transitionToS() {
    uint16_t mappedBrightness = map(maxBrightness, 0, 100, 0, (1 << ledController.getResolution()) - 1);
    ledController.startFadeTo(mappedBrightness, 1000);
    stateDuration = 5 * 60 * 1000; // 5 minuti
}

void LampStateMachine::transitionToRS() {
    uint16_t mappedBrightness = map(maxBrightness, 0, 100, 0, ((1 << ledController.getResolution()) - 1) * 7 / 10);
    ledController.startFadeTo(mappedBrightness, 2000);
    stateDuration = 15 * 60 * 1000; // 15 minuti
}

void LampStateMachine::transitionToSL() {
    uint16_t mappedBrightness = map(maxBrightness, 0, 100, 0, ((1 << ledController.getResolution()) - 1) * 3 / 10);
    ledController.startFadeTo(mappedBrightness, 3000);
    stateDuration = 30 * 60 * 1000; // 30 minuti
}

void LampStateMachine::transitionToSP() {
    ledController.startFadeOut(5000);
    stateDuration = UINT32_MAX; // Nessun timeout per lo stato SP
}

void LampStateMachine::transitionToR() {
    uint16_t mappedBrightness = map(maxBrightness, 0, 100, 0, ((1 << ledController.getResolution()) - 1) * 5 / 10);
    ledController.startFadeTo(mappedBrightness, 15 * 60 * 1000); // 15 minuti di fade per simulare l'alba
    stateDuration = 20 * 60 * 1000; // 20 minuti
}

void LampStateMachine::transitionToA() {
    // Implementare una sequenza di lampeggio o un'indicazione visiva dell'anomalia
    stateDuration = 60 * 1000; // 1 minuto, poi tenta di tornare a NP
}

const char* LampStateMachine::getStateName(LampState state) {
    switch (state) {
        case LampState::NP: return "NonPresente";
        case LampState::S: return "Sveglio";
        case LampState::RS: return "Relax/Pre-Sonno";
        case LampState::SL: return "Sonno Leggero";
        case LampState::SP: return "Sonno Profondo";
        case LampState::R: return "Risveglio";
        case LampState::A: return "Anomalia";
        default: return "Unknown";
    }
}