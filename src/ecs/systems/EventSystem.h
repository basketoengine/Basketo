#pragma once

#include "../System.h"
#include "../ComponentManager.h"
#include "../components/EventComponent.h"
#include "../components/TransformComponent.h"
#include <queue>
#include <unordered_map>
#include <chrono>
#include <iostream>

class EventSystem : public System {
public:
    EventSystem();
    ~EventSystem() = default;
    
    void update(ComponentManager* componentManager, float deltaTime);
    
    // Global event broadcasting
    void broadcastEvent(const EventData& event);
    void broadcastEvent(EventType type, Entity sender, const std::string& eventName = "");
    void broadcastCustomEvent(const std::string& eventName, Entity sender);
    
    // Targeted event sending
    void sendEventToEntity(Entity target, const EventData& event);
    void sendEventToEntity(Entity target, EventType type, Entity sender, const std::string& eventName = "");
    
    // Event filtering and querying
    std::vector<EventData> getEventsOfType(EventType type) const;
    std::vector<EventData> getEventsWithName(const std::string& eventName) const;
    std::vector<EventData> getEventsFromSender(Entity sender) const;
    
    // Performance metrics
    struct PerformanceMetrics {
        int totalEventsProcessed = 0;
        int eventsThisFrame = 0;
        int listenersTriggered = 0;
        int eventQueueSize = 0;
        float processingTime = 0.0f;
        int globalBroadcasts = 0;
        int targetedEvents = 0;
    };
    
    const PerformanceMetrics& getMetrics() const { return metrics; }
    void resetMetrics();
    
    // Event system configuration
    void setMaxEventsPerFrame(int maxEvents) { maxEventsPerFrame = maxEvents; }
    void setEventHistorySize(int size) { maxEventHistorySize = size; }
    void enableDebugLogging(bool enable) { debugLogging = enable; }
    
    // Built-in event generators
    void generateCollisionEvents(ComponentManager* componentManager);
    void generateInputEvents(ComponentManager* componentManager);
    void generateTimerEvents(ComponentManager* componentManager, float deltaTime);
    
private:
    // Event processing
    void processOutgoingEvents(ComponentManager* componentManager);
    void deliverEvent(const EventData& event, ComponentManager* componentManager);
    void processEventQueue(ComponentManager* componentManager);
    
    // Event validation and filtering
    bool isValidEvent(const EventData& event) const;
    bool shouldDeliverEvent(const EventData& event, Entity target, const EventListenerRegistration& listener) const;
    
    // Internal event queue for frame-based processing
    std::queue<EventData> eventQueue;
    std::vector<EventData> eventHistory;
    
    // Configuration
    int maxEventsPerFrame = 1000;
    int maxEventHistorySize = 500;
    bool debugLogging = false;
    
    // Performance tracking
    PerformanceMetrics metrics;
    std::chrono::high_resolution_clock::time_point frameStartTime;
    
    // Event ID generation
    static int nextEventId;
    int generateEventId() { return ++nextEventId; }
    
    // Helper methods
    void logEvent(const EventData& event, const std::string& action) const;
    void addToHistory(const EventData& event);
    void cleanupExpiredEvents();
};

// Global event system utilities
namespace EventUtils {
    // Event creation helpers
    EventData createCollisionEvent(Entity entity1, Entity entity2, const std::string& collisionType = "");
    EventData createInputEvent(const std::string& inputName, bool pressed, Entity target = NO_ENTITY);
    EventData createTimerEvent(Entity entity, const std::string& timerName, float duration);
    EventData createCustomEvent(const std::string& eventName, Entity sender, Entity target = NO_ENTITY);
    
    // Event parameter helpers
    void addPositionParameter(EventData& event, float x, float y);
    void addVelocityParameter(EventData& event, float vx, float vy);
    void addHealthParameter(EventData& event, float health, float maxHealth);
    void addScoreParameter(EventData& event, int score, int delta = 0);
    
    // Event type conversion
    std::string eventTypeToString(EventType type);
    EventType stringToEventType(const std::string& typeStr);
}
