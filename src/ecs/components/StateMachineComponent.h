#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include "../Entity.h"
#include "EventComponent.h"
#include "../../../vendor/nlohmann/json.hpp"

// Forward declarations
class StateMachineSystem;

// State transition condition types
enum class TransitionCondition {
    ALWAYS,
    ON_EVENT,
    ON_TIMER,
    ON_PARAMETER,
    ON_SCRIPT_CONDITION
};

// State transition definition
struct StateTransition {
    std::string fromState;
    std::string toState;
    TransitionCondition condition;
    
    // Condition parameters
    std::string eventName;
    EventType eventType = EventType::CUSTOM_EVENT;
    float timerDuration = 0.0f;
    std::string parameterName;
    std::string parameterValue;
    std::string scriptCondition; // Lua script for custom conditions
    
    // Transition properties
    float delay = 0.0f; // Delay before transition
    bool interruptible = true; // Can this transition be interrupted
    int priority = 0; // Higher priority transitions take precedence
    
    StateTransition() = default;
    
    StateTransition(const std::string& from, const std::string& to, TransitionCondition cond)
        : fromState(from), toState(to), condition(cond) {}
};

// State definition with callbacks
struct State {
    std::string name;
    
    // State callbacks (can be set via Lua or C++)
    std::function<void(Entity)> onEnter;
    std::function<void(Entity, float)> onUpdate;
    std::function<void(Entity)> onExit;
    
    // State properties
    std::unordered_map<std::string, std::string> parameters;
    float minDuration = 0.0f; // Minimum time to stay in this state
    float maxDuration = -1.0f; // Maximum time (-1 = infinite)
    
    // Animation integration
    std::string animationName;
    bool loopAnimation = true;
    
    // Audio integration
    std::string enterSoundId;
    std::string exitSoundId;
    std::string loopSoundId;
    
    State() = default;
    State(const std::string& stateName) : name(stateName) {}
    
    // Helper methods for parameters
    void setParameter(const std::string& key, const std::string& value) {
        parameters[key] = value;
    }
    
    std::string getParameter(const std::string& key, const std::string& defaultValue = "") const {
        auto it = parameters.find(key);
        return (it != parameters.end()) ? it->second : defaultValue;
    }
};

// State machine component
struct StateMachineComponent {
    // States
    std::unordered_map<std::string, State> states;
    std::vector<StateTransition> transitions;
    
    // Current state
    std::string currentState;
    std::string previousState;
    float currentStateTime = 0.0f;
    float transitionDelay = 0.0f;
    bool inTransition = false;
    
    // State machine properties
    std::string initialState;
    bool enabled = true;
    bool debugMode = false; // Log state changes
    
    // State history for debugging
    std::vector<std::string> stateHistory;
    int maxHistorySize = 20;
    
    // Performance tracking
    int transitionsThisFrame = 0;
    int stateUpdatesThisFrame = 0;
    
    StateMachineComponent() = default;
    
    // State management
    void addState(const State& state) {
        states[state.name] = state;
        if (initialState.empty()) {
            initialState = state.name;
        }
    }
    
    void addState(const std::string& name) {
        State state(name);
        addState(state);
    }
    
    void removeState(const std::string& name) {
        states.erase(name);
        // Remove transitions involving this state
        transitions.erase(
            std::remove_if(transitions.begin(), transitions.end(),
                [&name](const StateTransition& t) {
                    return t.fromState == name || t.toState == name;
                }),
            transitions.end());
    }
    
    // Transition management
    void addTransition(const StateTransition& transition) {
        transitions.push_back(transition);
        // Sort by priority (higher first)
        std::sort(transitions.begin(), transitions.end(),
            [](const StateTransition& a, const StateTransition& b) {
                return a.priority > b.priority;
            });
    }
    
    void addTransition(const std::string& from, const std::string& to, TransitionCondition condition) {
        StateTransition transition(from, to, condition);
        addTransition(transition);
    }
    
    // Event-based transition
    void addEventTransition(const std::string& from, const std::string& to, const std::string& eventName) {
        StateTransition transition(from, to, TransitionCondition::ON_EVENT);
        transition.eventName = eventName;
        addTransition(transition);
    }
    
    void addEventTransition(const std::string& from, const std::string& to, EventType eventType) {
        StateTransition transition(from, to, TransitionCondition::ON_EVENT);
        transition.eventType = eventType;
        addTransition(transition);
    }
    
    // Timer-based transition
    void addTimerTransition(const std::string& from, const std::string& to, float duration) {
        StateTransition transition(from, to, TransitionCondition::ON_TIMER);
        transition.timerDuration = duration;
        addTransition(transition);
    }
    
    // Get current state
    const State* getCurrentState() const {
        auto it = states.find(currentState);
        return (it != states.end()) ? &it->second : nullptr;
    }
    
    State* getCurrentState() {
        auto it = states.find(currentState);
        return (it != states.end()) ? &it->second : nullptr;
    }
    
    // Check if state exists
    bool hasState(const std::string& name) const {
        return states.find(name) != states.end();
    }
    
    // Initialize state machine
    void initialize() {
        if (!initialState.empty() && hasState(initialState)) {
            currentState = initialState;
            currentStateTime = 0.0f;
            addToHistory(currentState);
        }
    }
    
    // Add state to history
    void addToHistory(const std::string& stateName) {
        stateHistory.push_back(stateName);
        if (stateHistory.size() > maxHistorySize) {
            stateHistory.erase(stateHistory.begin());
        }
    }
    
    // Reset counters
    void resetFrameCounters() {
        transitionsThisFrame = 0;
        stateUpdatesThisFrame = 0;
    }
};

// Built-in state machine templates
namespace StateMachineTemplates {
    StateMachineComponent createPlayerController();
    StateMachineComponent createEnemyAI();
    StateMachineComponent createUIStateMachine();
    StateMachineComponent createGameFlowStateMachine();
}

// JSON serialization
inline void to_json(nlohmann::json& j, const StateMachineComponent& comp) {
    j = nlohmann::json{
        {"currentState", comp.currentState},
        {"initialState", comp.initialState},
        {"enabled", comp.enabled},
        {"debugMode", comp.debugMode},
        {"maxHistorySize", comp.maxHistorySize}
    };
    
    // Serialize states (basic properties only)
    nlohmann::json statesJson;
    for (const auto& [name, state] : comp.states) {
        statesJson[name] = {
            {"name", state.name},
            {"parameters", state.parameters},
            {"minDuration", state.minDuration},
            {"maxDuration", state.maxDuration},
            {"animationName", state.animationName},
            {"loopAnimation", state.loopAnimation},
            {"enterSoundId", state.enterSoundId},
            {"exitSoundId", state.exitSoundId},
            {"loopSoundId", state.loopSoundId}
        };
    }
    j["states"] = statesJson;
    
    // Serialize transitions
    nlohmann::json transitionsJson;
    for (const auto& transition : comp.transitions) {
        transitionsJson.push_back({
            {"fromState", transition.fromState},
            {"toState", transition.toState},
            {"condition", static_cast<int>(transition.condition)},
            {"eventName", transition.eventName},
            {"eventType", static_cast<int>(transition.eventType)},
            {"timerDuration", transition.timerDuration},
            {"parameterName", transition.parameterName},
            {"parameterValue", transition.parameterValue},
            {"scriptCondition", transition.scriptCondition},
            {"delay", transition.delay},
            {"interruptible", transition.interruptible},
            {"priority", transition.priority}
        });
    }
    j["transitions"] = transitionsJson;
}

inline void from_json(const nlohmann::json& j, StateMachineComponent& comp) {
    comp.currentState = j.value("currentState", "");
    comp.initialState = j.value("initialState", "");
    comp.enabled = j.value("enabled", true);
    comp.debugMode = j.value("debugMode", false);
    comp.maxHistorySize = j.value("maxHistorySize", 20);
    
    // Deserialize states
    if (j.contains("states")) {
        for (const auto& [name, stateJson] : j["states"].items()) {
            State state;
            state.name = stateJson.value("name", name);
            state.parameters = stateJson.value("parameters", std::unordered_map<std::string, std::string>{});
            state.minDuration = stateJson.value("minDuration", 0.0f);
            state.maxDuration = stateJson.value("maxDuration", -1.0f);
            state.animationName = stateJson.value("animationName", "");
            state.loopAnimation = stateJson.value("loopAnimation", true);
            state.enterSoundId = stateJson.value("enterSoundId", "");
            state.exitSoundId = stateJson.value("exitSoundId", "");
            state.loopSoundId = stateJson.value("loopSoundId", "");
            comp.states[name] = state;
        }
    }
    
    // Deserialize transitions
    if (j.contains("transitions")) {
        for (const auto& transitionJson : j["transitions"]) {
            StateTransition transition;
            transition.fromState = transitionJson.value("fromState", "");
            transition.toState = transitionJson.value("toState", "");
            transition.condition = static_cast<TransitionCondition>(transitionJson.value("condition", 0));
            transition.eventName = transitionJson.value("eventName", "");
            transition.eventType = static_cast<EventType>(transitionJson.value("eventType", 0));
            transition.timerDuration = transitionJson.value("timerDuration", 0.0f);
            transition.parameterName = transitionJson.value("parameterName", "");
            transition.parameterValue = transitionJson.value("parameterValue", "");
            transition.scriptCondition = transitionJson.value("scriptCondition", "");
            transition.delay = transitionJson.value("delay", 0.0f);
            transition.interruptible = transitionJson.value("interruptible", true);
            transition.priority = transitionJson.value("priority", 0);
            comp.transitions.push_back(transition);
        }
    }
}

// State machine template implementations
namespace StateMachineTemplates {
    inline StateMachineComponent createPlayerController() {
        StateMachineComponent sm;
        sm.initialState = "Idle";

        // Define states
        State idle("Idle");
        idle.animationName = "player_idle";
        idle.loopAnimation = true;
        sm.addState(idle);

        State walking("Walking");
        walking.animationName = "player_walk";
        walking.loopAnimation = true;
        sm.addState(walking);

        State jumping("Jumping");
        jumping.animationName = "player_jump";
        jumping.loopAnimation = false;
        jumping.minDuration = 0.5f;
        sm.addState(jumping);

        State attacking("Attacking");
        attacking.animationName = "player_attack";
        attacking.loopAnimation = false;
        attacking.minDuration = 0.3f;
        attacking.enterSoundId = "attack_sound";
        sm.addState(attacking);

        // Define transitions
        sm.addEventTransition("Idle", "Walking", "move_input");
        sm.addEventTransition("Walking", "Idle", "stop_input");
        sm.addEventTransition("Idle", "Jumping", "jump_input");
        sm.addEventTransition("Walking", "Jumping", "jump_input");
        sm.addTimerTransition("Jumping", "Idle", 1.0f);
        sm.addEventTransition("Idle", "Attacking", "attack_input");
        sm.addTimerTransition("Attacking", "Idle", 0.5f);

        return sm;
    }

    inline StateMachineComponent createEnemyAI() {
        StateMachineComponent sm;
        sm.initialState = "Patrol";

        // Define states
        State patrol("Patrol");
        patrol.animationName = "enemy_walk";
        patrol.loopAnimation = true;
        sm.addState(patrol);

        State chase("Chase");
        chase.animationName = "enemy_run";
        chase.loopAnimation = true;
        sm.addState(chase);

        State attack("Attack");
        attack.animationName = "enemy_attack";
        attack.loopAnimation = false;
        attack.minDuration = 0.8f;
        attack.enterSoundId = "enemy_attack";
        sm.addState(attack);

        State flee("Flee");
        flee.animationName = "enemy_run";
        flee.loopAnimation = true;
        flee.maxDuration = 3.0f;
        sm.addState(flee);

        // Define transitions
        sm.addEventTransition("Patrol", "Chase", "player_detected");
        sm.addEventTransition("Chase", "Attack", "player_in_range");
        sm.addEventTransition("Attack", "Chase", "attack_complete");
        sm.addEventTransition("Chase", "Patrol", "player_lost");
        sm.addEventTransition("Chase", "Flee", "low_health");
        sm.addEventTransition("Attack", "Flee", "low_health");
        sm.addTimerTransition("Flee", "Patrol", 3.0f);

        return sm;
    }

    inline StateMachineComponent createUIStateMachine() {
        StateMachineComponent sm;
        sm.initialState = "MainMenu";

        // Define states
        sm.addState("MainMenu");
        sm.addState("Settings");
        sm.addState("GamePlay");
        sm.addState("Pause");
        sm.addState("GameOver");

        // Define transitions
        sm.addEventTransition("MainMenu", "Settings", "settings_button");
        sm.addEventTransition("Settings", "MainMenu", "back_button");
        sm.addEventTransition("MainMenu", "GamePlay", "play_button");
        sm.addEventTransition("GamePlay", "Pause", "pause_input");
        sm.addEventTransition("Pause", "GamePlay", "resume_button");
        sm.addEventTransition("Pause", "MainMenu", "quit_button");
        sm.addEventTransition("GamePlay", "GameOver", "player_died");
        sm.addEventTransition("GameOver", "MainMenu", "restart_button");

        return sm;
    }

    inline StateMachineComponent createGameFlowStateMachine() {
        StateMachineComponent sm;
        sm.initialState = "Loading";

        // Define states
        sm.addState("Loading");
        sm.addState("Playing");
        sm.addState("Paused");
        sm.addState("GameOver");
        sm.addState("Victory");

        // Define transitions
        sm.addTimerTransition("Loading", "Playing", 2.0f);
        sm.addEventTransition("Playing", "Paused", "pause_game");
        sm.addEventTransition("Paused", "Playing", "resume_game");
        sm.addEventTransition("Playing", "GameOver", "game_over");
        sm.addEventTransition("Playing", "Victory", "level_complete");
        sm.addEventTransition("GameOver", "Loading", "restart_game");
        sm.addEventTransition("Victory", "Loading", "next_level");

        return sm;
    }
}
