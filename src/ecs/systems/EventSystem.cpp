#include "EventSystem.h"
#include <algorithm>
#include <chrono>

int EventSystem::nextEventId = 0;

EventSystem::EventSystem() {
    std::cout << "[EventSystem] Initialized" << std::endl;
}

void EventSystem::update(ComponentManager* componentManager, float deltaTime) {
    frameStartTime = std::chrono::high_resolution_clock::now();
    
    // Reset frame metrics
    metrics.eventsThisFrame = 0;
    metrics.listenersTriggered = 0;
    metrics.globalBroadcasts = 0;
    metrics.targetedEvents = 0;
    
    // Process outgoing events from all entities
    processOutgoingEvents(componentManager);
    
    // Process the global event queue
    processEventQueue(componentManager);
    
    // Generate built-in events
    generateTimerEvents(componentManager, deltaTime);
    
    // Clean up expired events
    cleanupExpiredEvents();
    
    // Update performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    metrics.processingTime = std::chrono::duration<float, std::milli>(endTime - frameStartTime).count();
    metrics.eventQueueSize = static_cast<int>(eventQueue.size());
    
    // Reset frame counters for all event components
    for (auto const& entity : entities) {
        if (componentManager->hasComponent<EventComponent>(entity)) {
            auto& eventComp = componentManager->getComponent<EventComponent>(entity);
            eventComp.eventsProcessedThisFrame = 0;
            eventComp.eventsSentThisFrame = 0;
        }
    }
}

void EventSystem::processOutgoingEvents(ComponentManager* componentManager) {
    for (auto const& entity : entities) {
        if (!componentManager->hasComponent<EventComponent>(entity)) continue;
        
        auto& eventComp = componentManager->getComponent<EventComponent>(entity);
        
        // Process all outgoing events from this entity
        for (auto& event : eventComp.outgoingEvents) {
            // Set sender if not already set
            if (event.sender == NO_ENTITY) {
                event.sender = entity;
            }
            
            // Set timestamp
            event.timestamp = std::chrono::duration<float>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            
            // Add to global event queue
            eventQueue.push(event);
            
            // Add to history
            addToHistory(event);
            eventComp.addToHistory(event);
            
            if (debugLogging) {
                logEvent(event, "SENT");
            }
        }
        
        // Clear outgoing events
        eventComp.outgoingEvents.clear();
    }
}

void EventSystem::processEventQueue(ComponentManager* componentManager) {
    int eventsProcessed = 0;
    
    while (!eventQueue.empty() && eventsProcessed < maxEventsPerFrame) {
        EventData event = eventQueue.front();
        eventQueue.pop();
        
        if (!isValidEvent(event)) {
            continue;
        }
        
        deliverEvent(event, componentManager);
        eventsProcessed++;
        metrics.eventsThisFrame++;
        metrics.totalEventsProcessed++;
    }
}

void EventSystem::deliverEvent(const EventData& event, ComponentManager* componentManager) {
    if (event.target != NO_ENTITY) {
        // Targeted event - deliver to specific entity
        if (componentManager->hasComponent<EventComponent>(event.target)) {
            auto& targetEventComp = componentManager->getComponent<EventComponent>(event.target);
            
            // Process listeners for this specific entity
            for (auto it = targetEventComp.listeners.begin(); it != targetEventComp.listeners.end();) {
                if (shouldDeliverEvent(event, event.target, *it)) {
                    try {
                        it->callback(event);
                        metrics.listenersTriggered++;
                        targetEventComp.eventsProcessedThisFrame++;
                        
                        if (debugLogging) {
                            logEvent(event, "DELIVERED_TO_TARGET");
                        }
                        
                        // Remove one-shot listeners
                        if (it->oneShot) {
                            it = targetEventComp.listeners.erase(it);
                            continue;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "[EventSystem] Error in event listener: " << e.what() << std::endl;
                    }
                }
                ++it;
            }
        }
        metrics.targetedEvents++;
    } else {
        // Broadcast event - deliver to all entities with listeners
        for (auto const& entity : entities) {
            if (!componentManager->hasComponent<EventComponent>(entity)) continue;
            
            auto& eventComp = componentManager->getComponent<EventComponent>(entity);
            
            // Process listeners for this entity
            for (auto it = eventComp.listeners.begin(); it != eventComp.listeners.end();) {
                if (shouldDeliverEvent(event, entity, *it)) {
                    try {
                        it->callback(event);
                        metrics.listenersTriggered++;
                        eventComp.eventsProcessedThisFrame++;
                        
                        if (debugLogging) {
                            logEvent(event, "BROADCAST_TO_" + std::to_string(entity));
                        }
                        
                        // Remove one-shot listeners
                        if (it->oneShot) {
                            it = eventComp.listeners.erase(it);
                            continue;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "[EventSystem] Error in event listener: " << e.what() << std::endl;
                    }
                }
                ++it;
            }
        }
        metrics.globalBroadcasts++;
    }
}

bool EventSystem::shouldDeliverEvent(const EventData& event, Entity target, const EventListenerRegistration& listener) const {
    // Check event type match
    if (listener.eventType != event.type) {
        return false;
    }
    
    // For custom events, check name match
    if (event.type == EventType::CUSTOM_EVENT) {
        if (listener.eventName != event.eventName) {
            return false;
        }
    }
    
    // Don't deliver to sender (unless explicitly targeting self)
    if (event.sender == target && event.target == NO_ENTITY) {
        return false;
    }
    
    return true;
}

bool EventSystem::isValidEvent(const EventData& event) const {
    // Basic validation
    if (event.consumed) {
        return false;
    }
    
    // Check if sender still exists (basic check)
    if (event.sender != NO_ENTITY) {
        // Could add entity existence check here if needed
    }
    
    return true;
}

void EventSystem::broadcastEvent(const EventData& event) {
    EventData broadcastEvent = event;
    broadcastEvent.target = NO_ENTITY; // Ensure it's a broadcast
    eventQueue.push(broadcastEvent);
    addToHistory(broadcastEvent);
}

void EventSystem::broadcastEvent(EventType type, Entity sender, const std::string& eventName) {
    EventData event(type, sender, NO_ENTITY, eventName);
    broadcastEvent(event);
}

void EventSystem::broadcastCustomEvent(const std::string& eventName, Entity sender) {
    EventData event(EventType::CUSTOM_EVENT, sender, NO_ENTITY, eventName);
    broadcastEvent(event);
}

void EventSystem::sendEventToEntity(Entity target, const EventData& event) {
    EventData targetedEvent = event;
    targetedEvent.target = target;
    eventQueue.push(targetedEvent);
    addToHistory(targetedEvent);
}

void EventSystem::sendEventToEntity(Entity target, EventType type, Entity sender, const std::string& eventName) {
    EventData event(type, sender, target, eventName);
    sendEventToEntity(target, event);
}

void EventSystem::logEvent(const EventData& event, const std::string& action) const {
    std::cout << "[EventSystem] " << action << " - Type: " << static_cast<int>(event.type) 
              << ", Name: " << event.eventName 
              << ", Sender: " << event.sender 
              << ", Target: " << event.target << std::endl;
}

void EventSystem::addToHistory(const EventData& event) {
    eventHistory.push_back(event);
    if (eventHistory.size() > maxEventHistorySize) {
        eventHistory.erase(eventHistory.begin());
    }
}

void EventSystem::cleanupExpiredEvents() {
    // Remove old events from history (could be based on timestamp)
    // For now, just maintain size limit
}

void EventSystem::resetMetrics() {
    metrics = PerformanceMetrics{};
}

void EventSystem::generateTimerEvents(ComponentManager* componentManager, float deltaTime) {
    // This would integrate with a timer system if we had one
    // For now, this is a placeholder for future timer-based events
}

void EventSystem::generateCollisionEvents(ComponentManager* componentManager) {
    // This would integrate with the collision system
    // Generate collision enter/exit events based on collision data
}

void EventSystem::generateInputEvents(ComponentManager* componentManager) {
    // This would integrate with the input system
    // Generate input events based on keyboard/mouse state
}

// Event utility functions
namespace EventUtils {
    EventData createCollisionEvent(Entity entity1, Entity entity2, const std::string& collisionType) {
        EventData event(EventType::COLLISION_ENTER, entity1, entity2, "collision");
        event.setParameter("collisionType", collisionType);
        event.setParameter("entity1", static_cast<int>(entity1));
        event.setParameter("entity2", static_cast<int>(entity2));
        return event;
    }
    
    EventData createInputEvent(const std::string& inputName, bool pressed, Entity target) {
        EventType type = pressed ? EventType::INPUT_KEY_DOWN : EventType::INPUT_KEY_UP;
        EventData event(type, NO_ENTITY, target, inputName);
        event.setParameter("key", inputName);
        event.setParameter("pressed", pressed ? "true" : "false");
        return event;
    }
    
    EventData createTimerEvent(Entity entity, const std::string& timerName, float duration) {
        EventData event(EventType::TIMER_EXPIRED, entity, entity, timerName);
        event.setParameter("timerName", timerName);
        event.setParameter("duration", duration);
        return event;
    }
    
    EventData createCustomEvent(const std::string& eventName, Entity sender, Entity target) {
        return EventData(EventType::CUSTOM_EVENT, sender, target, eventName);
    }
    
    void addPositionParameter(EventData& event, float x, float y) {
        event.setParameter("x", x);
        event.setParameter("y", y);
    }
    
    void addVelocityParameter(EventData& event, float vx, float vy) {
        event.setParameter("vx", vx);
        event.setParameter("vy", vy);
    }
    
    void addHealthParameter(EventData& event, float health, float maxHealth) {
        event.setParameter("health", health);
        event.setParameter("maxHealth", maxHealth);
        event.setParameter("healthPercent", (health / maxHealth) * 100.0f);
    }
    
    void addScoreParameter(EventData& event, int score, int delta) {
        event.setParameter("score", score);
        event.setParameter("scoreDelta", delta);
    }
    
    std::string eventTypeToString(EventType type) {
        switch (type) {
            case EventType::COLLISION_ENTER: return "COLLISION_ENTER";
            case EventType::COLLISION_EXIT: return "COLLISION_EXIT";
            case EventType::INPUT_KEY_DOWN: return "INPUT_KEY_DOWN";
            case EventType::INPUT_KEY_UP: return "INPUT_KEY_UP";
            case EventType::CUSTOM_EVENT: return "CUSTOM_EVENT";
            case EventType::STATE_ENTER: return "STATE_ENTER";
            case EventType::STATE_EXIT: return "STATE_EXIT";
            case EventType::ANIMATION_COMPLETE: return "ANIMATION_COMPLETE";
            case EventType::TIMER_EXPIRED: return "TIMER_EXPIRED";
            case EventType::HEALTH_CHANGED: return "HEALTH_CHANGED";
            case EventType::SCORE_CHANGED: return "SCORE_CHANGED";
            default: return "UNKNOWN";
        }
    }
    
    EventType stringToEventType(const std::string& typeStr) {
        if (typeStr == "COLLISION_ENTER") return EventType::COLLISION_ENTER;
        if (typeStr == "COLLISION_EXIT") return EventType::COLLISION_EXIT;
        if (typeStr == "INPUT_KEY_DOWN") return EventType::INPUT_KEY_DOWN;
        if (typeStr == "INPUT_KEY_UP") return EventType::INPUT_KEY_UP;
        if (typeStr == "STATE_ENTER") return EventType::STATE_ENTER;
        if (typeStr == "STATE_EXIT") return EventType::STATE_EXIT;
        if (typeStr == "ANIMATION_COMPLETE") return EventType::ANIMATION_COMPLETE;
        if (typeStr == "TIMER_EXPIRED") return EventType::TIMER_EXPIRED;
        if (typeStr == "HEALTH_CHANGED") return EventType::HEALTH_CHANGED;
        if (typeStr == "SCORE_CHANGED") return EventType::SCORE_CHANGED;
        return EventType::CUSTOM_EVENT;
    }
}
