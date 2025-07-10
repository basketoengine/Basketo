#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include "../Entity.h"
#include "../../../vendor/nlohmann/json.hpp"

// Forward declarations
class EventSystem;

// Event types enumeration
enum class EventType {
    COLLISION_ENTER,
    COLLISION_EXIT,
    INPUT_KEY_DOWN,
    INPUT_KEY_UP,
    CUSTOM_EVENT,
    STATE_ENTER,
    STATE_EXIT,
    ANIMATION_COMPLETE,
    TIMER_EXPIRED,
    HEALTH_CHANGED,
    SCORE_CHANGED
};

// Base event data structure
struct EventData {
    EventType type;
    Entity sender;
    Entity target;
    std::string eventName;
    std::unordered_map<std::string, std::string> parameters;
    float timestamp;
    bool consumed = false;

    EventData() : type(EventType::CUSTOM_EVENT), sender(NO_ENTITY), target(NO_ENTITY), timestamp(0.0f) {}
    
    EventData(EventType t, Entity s, Entity tgt = NO_ENTITY, const std::string& name = "") 
        : type(t), sender(s), target(tgt), eventName(name), timestamp(0.0f) {}

    // Helper methods for parameters
    void setParameter(const std::string& key, const std::string& value) {
        parameters[key] = value;
    }

    void setParameter(const std::string& key, float value) {
        parameters[key] = std::to_string(value);
    }

    void setParameter(const std::string& key, int value) {
        parameters[key] = std::to_string(value);
    }

    std::string getParameter(const std::string& key, const std::string& defaultValue = "") const {
        auto it = parameters.find(key);
        return (it != parameters.end()) ? it->second : defaultValue;
    }

    float getParameterFloat(const std::string& key, float defaultValue = 0.0f) const {
        auto it = parameters.find(key);
        return (it != parameters.end()) ? std::stof(it->second) : defaultValue;
    }

    int getParameterInt(const std::string& key, int defaultValue = 0) const {
        auto it = parameters.find(key);
        return (it != parameters.end()) ? std::stoi(it->second) : defaultValue;
    }
};

// Event listener function type
using EventListener = std::function<void(const EventData&)>;

// Event listener registration
struct EventListenerRegistration {
    EventType eventType;
    std::string eventName;
    EventListener callback;
    int priority = 0; // Higher priority listeners are called first
    bool oneShot = false; // If true, listener is removed after first call

    EventListenerRegistration(EventType type, EventListener cb, int prio = 0, bool once = false)
        : eventType(type), callback(cb), priority(prio), oneShot(once) {}

    EventListenerRegistration(const std::string& name, EventListener cb, int prio = 0, bool once = false)
        : eventType(EventType::CUSTOM_EVENT), eventName(name), callback(cb), priority(prio), oneShot(once) {}
};

// Component for entities that can send and receive events
struct EventComponent {
    // Outgoing events queue (events this entity wants to send)
    std::vector<EventData> outgoingEvents;
    
    // Event listeners (events this entity wants to receive)
    std::vector<EventListenerRegistration> listeners;
    
    // Event history for debugging
    std::vector<EventData> eventHistory;
    int maxHistorySize = 50;
    
    // Performance tracking
    int eventsProcessedThisFrame = 0;
    int eventsSentThisFrame = 0;
    
    EventComponent() = default;

    // Send an event
    void sendEvent(const EventData& event) {
        outgoingEvents.push_back(event);
        eventsSentThisFrame++;
    }

    void sendEvent(EventType type, Entity target = NO_ENTITY, const std::string& name = "") {
        EventData event(type, NO_ENTITY, target, name); // sender will be set by EventSystem
        sendEvent(event);
    }

    void sendCustomEvent(const std::string& eventName, Entity target = NO_ENTITY) {
        EventData event(EventType::CUSTOM_EVENT, NO_ENTITY, target, eventName);
        sendEvent(event);
    }

    // Register event listener
    void addEventListener(EventType type, EventListener callback, int priority = 0, bool oneShot = false) {
        listeners.emplace_back(type, callback, priority, oneShot);
        // Sort by priority (higher first)
        std::sort(listeners.begin(), listeners.end(), 
            [](const EventListenerRegistration& a, const EventListenerRegistration& b) {
                return a.priority > b.priority;
            });
    }

    void addEventListener(const std::string& eventName, EventListener callback, int priority = 0, bool oneShot = false) {
        listeners.emplace_back(eventName, callback, priority, oneShot);
        // Sort by priority (higher first)
        std::sort(listeners.begin(), listeners.end(), 
            [](const EventListenerRegistration& a, const EventListenerRegistration& b) {
                return a.priority > b.priority;
            });
    }

    // Remove event listeners
    void removeEventListener(EventType type) {
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                [type](const EventListenerRegistration& reg) {
                    return reg.eventType == type;
                }),
            listeners.end());
    }

    void removeEventListener(const std::string& eventName) {
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                [&eventName](const EventListenerRegistration& reg) {
                    return reg.eventName == eventName;
                }),
            listeners.end());
    }

    // Clear all events and reset counters
    void clearEvents() {
        outgoingEvents.clear();
        eventsProcessedThisFrame = 0;
        eventsSentThisFrame = 0;
    }

    // Add event to history
    void addToHistory(const EventData& event) {
        eventHistory.push_back(event);
        if (eventHistory.size() > maxHistorySize) {
            eventHistory.erase(eventHistory.begin());
        }
    }
};

// JSON serialization for EventComponent (for scene persistence)
inline void to_json(nlohmann::json& j, const EventComponent& comp) {
    j = nlohmann::json{
        {"maxHistorySize", comp.maxHistorySize}
        // Note: listeners and events are runtime-only, not serialized
    };
}

inline void from_json(const nlohmann::json& j, EventComponent& comp) {
    comp.maxHistorySize = j.value("maxHistorySize", 50);
    comp.listeners.clear();
    comp.outgoingEvents.clear();
    comp.eventHistory.clear();
}
