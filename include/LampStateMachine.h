#pragma once

#include <Arduino.h>
#include <array>
#include <functional>
#include "LedController.h"
#include "MotionSensor.h"
#include "EnergyThresholds.h"
#include "Logger.h"

enum class LampState {
    NP,  // Nessuna Presenza
    S,   // Attivo/Sveglio
    RS,  // Relax/Pre-Sonno
    SL,  // Sonno Leggero
    SP,  // Sonno Profondo
    R,   // Risveglio
    A,   // Anomalia
    STATE_COUNT
};

class LampStateMachine {
private:
    static LampStateMachine* instance;

    LampState currentState;
    LedController& ledController;
    MotionSensor& motionSensor;
    EnergyThresholds& energyThresholds;
    uint8_t maxBrightness;
    uint32_t stateStartTime;
    uint32_t stateDuration;
    uint32_t debounceDelay;

    std::array<std::function<LampState(float, bool, bool, bool)>, static_cast<size_t>(LampState::STATE_COUNT)> stateTransitionRules;

    LampStateMachine(LedController& led, MotionSensor& motion, EnergyThresholds& thresholds);

    void initializeStateTransitionRules();
    void setState(LampState newState);

    // Transition functions
    void transitionToNP();
    void transitionToS();
    void transitionToRS();
    void transitionToSL();
    void transitionToSP();
    void transitionToR();
    void transitionToA();
    const char* getStateName(LampState state);


public:
    static LampStateMachine& getInstance(LedController& led, MotionSensor& motion, EnergyThresholds& thresholds);
    
    // Delete copy constructor and assignment operator
    LampStateMachine(const LampStateMachine&) = delete;
    LampStateMachine& operator=(const LampStateMachine&) = delete;

    void update(uint8_t maxBrightness, bool IsOnAutoMode);
    LampState getCurrentState() const { return currentState; }
};