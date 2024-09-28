#pragma once
#include <Arduino.h>
#include "LedController.h"
#include "MotionSensor.h"
#include <functional>
#include <array>

enum class LampState {
    OFF,
    FULL_ON,
    RELAXATION,
    SLEEP,
    SUDDEN_MOVEMENT,
    STATE_COUNT
};

class LampStateMachine {
private:
    LampState currentState;
    LedController& ledController;
    MotionSensor& motionSensor;
    uint8_t maxBrightness;
    unsigned long stateStartTime;
    unsigned long stateDuration;
    unsigned long lastMovementTime;
    unsigned long lastPresenceTime;
    unsigned long debounceDelay;

    // Costruttore privato per il pattern Singleton
    LampStateMachine(LedController& led, MotionSensor& motion);

    // Disabilitare il costruttore di copia e l'operatore di assegnazione
    LampStateMachine(const LampStateMachine&) = delete;
    LampStateMachine& operator=(const LampStateMachine&) = delete;

    void setState(LampState newState);

    // Funzioni di transizione
    void transitionToOff();
    void transitionToFullOn();
    void transitionToRelaxation();
    void transitionToSleep();
    void transitionToSuddenMovement();

    // Tabella di transizione degli stati
    using StateTransitionRule = std::function<LampState(bool isMovement, bool isPresence, bool stateTimedOut)>;
    std::array<StateTransitionRule, static_cast<size_t>(LampState::STATE_COUNT)> stateTransitionRules;

    void initializeStateTransitionRules();

public:
    // Metodo statico per ottenere l'istanza Singleton
    static LampStateMachine& getInstance(LedController& led, MotionSensor& motion) {
        static LampStateMachine instance(led, motion);  // Viene creata solo una volta
        return instance;
    }

    void update(uint8_t maxBrightness, bool IsOnAutoMode);
    LampState getCurrentState() const { return currentState; }
};