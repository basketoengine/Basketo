#pragma once

#include "../System.h"
#include "../ComponentManager.h"
#include "../components/StateMachineComponent.h"
#include "../components/EventComponent.h"
#include "../components/AnimationComponent.h"
#include "../components/AudioComponent.h"
#include "EventSystem.h"
#include <iostream>
#include <chrono>

class StateMachineSystem : public System {
public:
    StateMachineSystem();
    ~StateMachineSystem() = default;
    
    void update(ComponentManager* componentManager, float deltaTime);
    
    // State machine control
    void changeState(Entity entity, const std::string& newState, ComponentManager* componentManager);
    void forceStateChange(Entity entity, const std::string& newState, ComponentManager* componentManager);
    
    // State queries
    std::string getCurrentState(Entity entity, ComponentManager* componentManager) const;
    bool isInState(Entity entity, const std::string& stateName, ComponentManager* componentManager) const;
    float getStateTime(Entity entity, ComponentManager* componentManager) const;
    
    // Transition control
    void enableTransitions(Entity entity, bool enabled, ComponentManager* componentManager);
    void addRuntimeTransition(Entity entity, const StateTransition& transition, ComponentManager* componentManager);
    
    // Performance metrics
    struct PerformanceMetrics {
        int totalStateMachines = 0;
        int activeStateMachines = 0;
        int stateTransitions = 0;
        int stateUpdates = 0;
        float processingTime = 0.0f;
        int eventTriggeredTransitions = 0;
        int timerTriggeredTransitions = 0;
    };
    
    const PerformanceMetrics& getMetrics() const { return metrics; }
    void resetMetrics();
    
    // Configuration
    void enableDebugLogging(bool enable) { debugLogging = enable; }
    void setMaxTransitionsPerFrame(int maxTransitions) { maxTransitionsPerFrame = maxTransitions; }
    
    // Integration with other systems
    void setEventSystem(EventSystem* eventSys) { eventSystem = eventSys; }
    
private:
    // State machine processing
    void updateStateMachine(Entity entity, ComponentManager* componentManager, float deltaTime);
    void processStateTransitions(Entity entity, ComponentManager* componentManager, float deltaTime);
    void executeStateTransition(Entity entity, const std::string& fromState, const std::string& toState, 
                               ComponentManager* componentManager);
    
    // Transition condition checking
    bool checkTransitionCondition(const StateTransition& transition, Entity entity, 
                                 ComponentManager* componentManager, float deltaTime);
    bool checkEventCondition(const StateTransition& transition, Entity entity, ComponentManager* componentManager);
    bool checkTimerCondition(const StateTransition& transition, Entity entity, ComponentManager* componentManager, float deltaTime);
    bool checkParameterCondition(const StateTransition& transition, Entity entity, ComponentManager* componentManager);
    bool checkScriptCondition(const StateTransition& transition, Entity entity, ComponentManager* componentManager);
    
    // State callbacks
    void executeStateEnter(Entity entity, const State& state, ComponentManager* componentManager);
    void executeStateUpdate(Entity entity, const State& state, ComponentManager* componentManager, float deltaTime);
    void executeStateExit(Entity entity, const State& state, ComponentManager* componentManager);
    
    // Integration helpers
    void updateAnimationForState(Entity entity, const State& state, ComponentManager* componentManager);
    void playAudioForState(Entity entity, const State& state, const std::string& audioType, ComponentManager* componentManager);
    void sendStateEvent(Entity entity, const std::string& stateName, EventType eventType, ComponentManager* componentManager);
    
    // Configuration
    bool debugLogging = false;
    int maxTransitionsPerFrame = 10;
    
    // System references
    EventSystem* eventSystem = nullptr;
    
    // Performance tracking
    PerformanceMetrics metrics;
    std::chrono::high_resolution_clock::time_point frameStartTime;
    
    // Helper methods
    void logStateChange(Entity entity, const std::string& fromState, const std::string& toState) const;
    bool isValidState(const StateMachineComponent& stateMachine, const std::string& stateName) const;
};
