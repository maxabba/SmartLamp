#include "LampStateMachine.h"

LampStateMachine::LampStateMachine(LedController& led, MotionSensor& motion)
    : currentState(LampState::OFF), ledController(led), motionSensor(motion) {
    initializeTransitionMatrix();
}

void LampStateMachine::begin() {
    esp_timer_create_args_t timerArgs = {
        .callback = &LampStateMachine::timerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "StateTimer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &stateTimer));
}

void LampStateMachine::timerCallback(void* arg) {
    LampStateMachine* machine = static_cast<LampStateMachine*>(arg);
    machine->handleStateTransition(machine->currentState);
}

void LampStateMachine::setState(LampState newState) {
    if (currentState != newState) {
        LampState oldState = currentState;
        currentState = newState;
        transitionMatrix[static_cast<size_t>(oldState)][static_cast<size_t>(newState)](*this);
    }
}

void LampStateMachine::handleStateTransition(LampState newState) {
    setState(newState);
}


void LampStateMachine::update() {
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

void LampStateMachine::transitionToOff() {
    ledController.startFadeOut(2000);
    esp_timer_stop(stateTimer);
}

void LampStateMachine::transitionToFullOn() {
    ledController.startFadeTo(4095, 1000);
    esp_timer_start_once(stateTimer, 5 * 60 * 1000000); // 5 minutes
}

void LampStateMachine::transitionToRelaxation() {
    ledController.startFadeTo(2048, 2000);
    esp_timer_start_once(stateTimer, 15 * 60 * 1000000); // 15 minutes
}

void LampStateMachine::transitionToSleep() {
    ledController.startFadeTo(512, 3000);
    esp_timer_stop(stateTimer);
}

void LampStateMachine::transitionToSuddenMovement() {
    ledController.startFadeTo(2048, 1000);
    esp_timer_start_once(stateTimer, 30 * 1000000); // 30 seconds
}

void LampStateMachine::initializeTransitionMatrix() {
    // Initialize all transitions to do nothing
    for (auto& row : transitionMatrix) {
        row.fill([](LampStateMachine&) {});
    }

    // Define valid transitions
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