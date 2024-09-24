#pragma once
#include <Arduino.h>
#include "esp_timer.h"
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
    STATE_COUNT  // Used to determine the size of our arrays
};

class LampStateMachine {
private:
    LampState currentState;
    LedController& ledController;
    MotionSensor& motionSensor;
    esp_timer_handle_t stateTimer;
    
    static void timerCallback(void* arg);
    void setState(LampState newState);

    using TransitionFunction = std::function<void(LampStateMachine&)>;
    std::array<std::array<TransitionFunction, static_cast<size_t>(LampState::STATE_COUNT)>, static_cast<size_t>(LampState::STATE_COUNT)> transitionMatrix;

    // Transition functions
    void transitionToOff();
    void transitionToFullOn();
    void transitionToRelaxation();
    void transitionToSleep();
    void transitionToSuddenMovement();

    // Helper function to set up the transition matrix
    void initializeTransitionMatrix();

public:
    LampStateMachine(LedController& led, MotionSensor& motion);
    void begin();
    void update();
    LampState getCurrentState() const { return currentState; }
    void handleStateTransition(LampState newState);
};
