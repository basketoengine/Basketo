#include "StateMachineSystem.h"
#include <algorithm>

StateMachineSystem::StateMachineSystem() {
    std::cout << "[StateMachineSystem] Initialized" << std::endl;
}

void StateMachineSystem::update(ComponentManager* componentManager, float deltaTime) {
    frameStartTime = std::chrono::high_resolution_clock::now();
    
    // Reset frame metrics
    metrics.stateTransitions = 0;
    metrics.stateUpdates = 0;
    metrics.eventTriggeredTransitions = 0;
    metrics.timerTriggeredTransitions = 0;
    metrics.totalStateMachines = 0;
    metrics.activeStateMachines = 0;
    
    // Process all state machines
    for (auto const& entity : entities) {
        if (!componentManager->hasComponent<StateMachineComponent>(entity)) continue;
        
        metrics.totalStateMachines++;
        
        auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
        
        if (!stateMachine.enabled) continue;
        
        metrics.activeStateMachines++;
        
        // Initialize state machine if needed
        if (stateMachine.currentState.empty() && !stateMachine.initialState.empty()) {
            stateMachine.initialize();
            if (!stateMachine.currentState.empty()) {
                executeStateEnter(entity, *stateMachine.getCurrentState(), componentManager);
                sendStateEvent(entity, stateMachine.currentState, EventType::STATE_ENTER, componentManager);
            }
        }
        
        // Update the state machine
        updateStateMachine(entity, componentManager, deltaTime);
        
        // Reset frame counters
        stateMachine.resetFrameCounters();
    }
    
    // Update performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    metrics.processingTime = std::chrono::duration<float, std::milli>(endTime - frameStartTime).count();
}

void StateMachineSystem::updateStateMachine(Entity entity, ComponentManager* componentManager, float deltaTime) {
    auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
    
    // Update current state time
    stateMachine.currentStateTime += deltaTime;
    
    // Handle transition delay
    if (stateMachine.inTransition) {
        stateMachine.transitionDelay -= deltaTime;
        if (stateMachine.transitionDelay <= 0.0f) {
            stateMachine.inTransition = false;
        }
        return; // Don't process transitions or state updates during transition delay
    }
    
    // Process state transitions
    processStateTransitions(entity, componentManager, deltaTime);
    
    // Update current state
    const State* currentState = stateMachine.getCurrentState();
    if (currentState) {
        executeStateUpdate(entity, *currentState, componentManager, deltaTime);
        updateAnimationForState(entity, *currentState, componentManager);
        metrics.stateUpdates++;
        stateMachine.stateUpdatesThisFrame++;
    }
}

void StateMachineSystem::processStateTransitions(Entity entity, ComponentManager* componentManager, float deltaTime) {
    auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
    
    if (stateMachine.currentState.empty()) return;
    
    int transitionsProcessed = 0;
    
    // Check all transitions from current state
    for (const auto& transition : stateMachine.transitions) {
        if (transitionsProcessed >= maxTransitionsPerFrame) break;
        
        // Only check transitions from current state
        if (transition.fromState != stateMachine.currentState) continue;
        
        // Check if transition condition is met
        if (checkTransitionCondition(transition, entity, componentManager, deltaTime)) {
            // Check minimum state duration
            const State* currentState = stateMachine.getCurrentState();
            if (currentState && stateMachine.currentStateTime < currentState->minDuration) {
                continue; // Haven't been in state long enough
            }
            
            // Execute the transition
            executeStateTransition(entity, transition.fromState, transition.toState, componentManager);
            
            // Set transition delay if specified
            if (transition.delay > 0.0f) {
                stateMachine.inTransition = true;
                stateMachine.transitionDelay = transition.delay;
            }
            
            transitionsProcessed++;
            metrics.stateTransitions++;
            stateMachine.transitionsThisFrame++;
            
            // Only process one transition per frame per entity
            break;
        }
    }
}

bool StateMachineSystem::checkTransitionCondition(const StateTransition& transition, Entity entity, 
                                                 ComponentManager* componentManager, float deltaTime) {
    switch (transition.condition) {
        case TransitionCondition::ALWAYS:
            return true;
            
        case TransitionCondition::ON_EVENT:
            return checkEventCondition(transition, entity, componentManager);
            
        case TransitionCondition::ON_TIMER:
            return checkTimerCondition(transition, entity, componentManager, deltaTime);
            
        case TransitionCondition::ON_PARAMETER:
            return checkParameterCondition(transition, entity, componentManager);
            
        case TransitionCondition::ON_SCRIPT_CONDITION:
            return checkScriptCondition(transition, entity, componentManager);
            
        default:
            return false;
    }
}

bool StateMachineSystem::checkEventCondition(const StateTransition& transition, Entity entity, ComponentManager* componentManager) {
    if (!componentManager->hasComponent<EventComponent>(entity)) return false;
    
    auto& eventComp = componentManager->getComponent<EventComponent>(entity);
    
    // Check recent events in history
    for (const auto& event : eventComp.eventHistory) {
        if (event.type == transition.eventType || 
            (event.type == EventType::CUSTOM_EVENT && event.eventName == transition.eventName)) {
            
            // Check if event is recent enough (within last few frames)
            float currentTime = std::chrono::duration<float>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            
            if (currentTime - event.timestamp < 0.1f) { // 100ms window
                metrics.eventTriggeredTransitions++;
                return true;
            }
        }
    }
    
    return false;
}

bool StateMachineSystem::checkTimerCondition(const StateTransition& transition, Entity entity, 
                                            ComponentManager* componentManager, float deltaTime) {
    auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
    
    if (stateMachine.currentStateTime >= transition.timerDuration) {
        metrics.timerTriggeredTransitions++;
        return true;
    }
    
    return false;
}

bool StateMachineSystem::checkParameterCondition(const StateTransition& transition, Entity entity, ComponentManager* componentManager) {
    auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
    
    const State* currentState = stateMachine.getCurrentState();
    if (!currentState) return false;
    
    std::string paramValue = currentState->getParameter(transition.parameterName);
    return paramValue == transition.parameterValue;
}

bool StateMachineSystem::checkScriptCondition(const StateTransition& transition, Entity entity, ComponentManager* componentManager) {
    // This would integrate with the script system to evaluate Lua conditions
    // For now, return false as placeholder
    return false;
}

void StateMachineSystem::executeStateTransition(Entity entity, const std::string& fromState, const std::string& toState, 
                                               ComponentManager* componentManager) {
    auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
    
    if (!isValidState(stateMachine, toState)) {
        if (debugLogging) {
            std::cerr << "[StateMachineSystem] Invalid state transition to: " << toState << std::endl;
        }
        return;
    }
    
    // Execute exit callback for current state
    const State* currentState = stateMachine.getCurrentState();
    if (currentState) {
        executeStateExit(entity, *currentState, componentManager);
        sendStateEvent(entity, fromState, EventType::STATE_EXIT, componentManager);
    }
    
    // Change state
    stateMachine.previousState = stateMachine.currentState;
    stateMachine.currentState = toState;
    stateMachine.currentStateTime = 0.0f;
    stateMachine.addToHistory(toState);
    
    // Execute enter callback for new state
    const State* newState = stateMachine.getCurrentState();
    if (newState) {
        executeStateEnter(entity, *newState, componentManager);
        sendStateEvent(entity, toState, EventType::STATE_ENTER, componentManager);
    }
    
    if (debugLogging) {
        logStateChange(entity, fromState, toState);
    }
}

void StateMachineSystem::executeStateEnter(Entity entity, const State& state, ComponentManager* componentManager) {
    if (state.onEnter) {
        try {
            state.onEnter(entity);
        } catch (const std::exception& e) {
            std::cerr << "[StateMachineSystem] Error in state enter callback: " << e.what() << std::endl;
        }
    }
    
    // Play enter sound
    if (!state.enterSoundId.empty()) {
        playAudioForState(entity, state, "enter", componentManager);
    }
}

void StateMachineSystem::executeStateUpdate(Entity entity, const State& state, ComponentManager* componentManager, float deltaTime) {
    if (state.onUpdate) {
        try {
            state.onUpdate(entity, deltaTime);
        } catch (const std::exception& e) {
            std::cerr << "[StateMachineSystem] Error in state update callback: " << e.what() << std::endl;
        }
    }
}

void StateMachineSystem::executeStateExit(Entity entity, const State& state, ComponentManager* componentManager) {
    if (state.onExit) {
        try {
            state.onExit(entity);
        } catch (const std::exception& e) {
            std::cerr << "[StateMachineSystem] Error in state exit callback: " << e.what() << std::endl;
        }
    }
    
    // Play exit sound
    if (!state.exitSoundId.empty()) {
        playAudioForState(entity, state, "exit", componentManager);
    }
}

void StateMachineSystem::updateAnimationForState(Entity entity, const State& state, ComponentManager* componentManager) {
    if (state.animationName.empty()) return;
    
    if (componentManager->hasComponent<AnimationComponent>(entity)) {
        auto& animComp = componentManager->getComponent<AnimationComponent>(entity);
        
        // Only change animation if it's different
        if (animComp.currentAnimationName != state.animationName) {
            animComp.play(state.animationName, true); // Force restart the animation
        }
    }
}

void StateMachineSystem::playAudioForState(Entity entity, const State& state, const std::string& audioType, ComponentManager* componentManager) {
    if (!componentManager->hasComponent<AudioComponent>(entity)) return;
    
    auto& audioComp = componentManager->getComponent<AudioComponent>(entity);
    
    std::string soundId;
    if (audioType == "enter") soundId = state.enterSoundId;
    else if (audioType == "exit") soundId = state.exitSoundId;
    else if (audioType == "loop") soundId = state.loopSoundId;
    
    if (!soundId.empty()) {
        // Check if it's a SoundEffectsComponent instead
        if (componentManager->hasComponent<SoundEffectsComponent>(entity)) {
            auto& soundEffectsComp = componentManager->getComponent<SoundEffectsComponent>(entity);
            soundEffectsComp.playQueue.push_back(soundId);
        } else {
            // For basic AudioComponent, just set the audioId
            audioComp.audioId = soundId;
            audioComp.playOnStart = true;
        }
    }
}

void StateMachineSystem::sendStateEvent(Entity entity, const std::string& stateName, EventType eventType, ComponentManager* componentManager) {
    if (!eventSystem || !componentManager->hasComponent<EventComponent>(entity)) return;
    
    EventData event(eventType, entity, NO_ENTITY, stateName);
    event.setParameter("stateName", stateName);
    event.setParameter("entity", static_cast<int>(entity));
    
    eventSystem->broadcastEvent(event);
}

void StateMachineSystem::changeState(Entity entity, const std::string& newState, ComponentManager* componentManager) {
    if (!componentManager->hasComponent<StateMachineComponent>(entity)) return;
    
    auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
    
    if (isValidState(stateMachine, newState)) {
        executeStateTransition(entity, stateMachine.currentState, newState, componentManager);
    }
}

void StateMachineSystem::forceStateChange(Entity entity, const std::string& newState, ComponentManager* componentManager) {
    // Same as changeState but bypasses transition conditions
    changeState(entity, newState, componentManager);
}

std::string StateMachineSystem::getCurrentState(Entity entity, ComponentManager* componentManager) const {
    if (!componentManager->hasComponent<StateMachineComponent>(entity)) return "";
    
    const auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
    return stateMachine.currentState;
}

bool StateMachineSystem::isInState(Entity entity, const std::string& stateName, ComponentManager* componentManager) const {
    return getCurrentState(entity, componentManager) == stateName;
}

float StateMachineSystem::getStateTime(Entity entity, ComponentManager* componentManager) const {
    if (!componentManager->hasComponent<StateMachineComponent>(entity)) return 0.0f;
    
    const auto& stateMachine = componentManager->getComponent<StateMachineComponent>(entity);
    return stateMachine.currentStateTime;
}

void StateMachineSystem::logStateChange(Entity entity, const std::string& fromState, const std::string& toState) const {
    std::cout << "[StateMachineSystem] Entity " << entity << " transitioned from '" 
              << fromState << "' to '" << toState << "'" << std::endl;
}

bool StateMachineSystem::isValidState(const StateMachineComponent& stateMachine, const std::string& stateName) const {
    return stateMachine.hasState(stateName);
}

void StateMachineSystem::resetMetrics() {
    metrics = PerformanceMetrics{};
}
