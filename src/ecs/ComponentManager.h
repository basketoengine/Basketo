#pragma once
#include "Component.h"
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <cassert>
#include "Types.h"

class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void entityDestroyed(Entity entity) = 0;
};

template<typename T>
class ComponentArray : public IComponentArray {
public:
    void insertData(Entity entity, T component) {
        assert(entityToIndex.find(entity) == entityToIndex.end());
        size_t newIndex = size;
        entityToIndex[entity] = newIndex;
        indexToEntity[newIndex] = entity;
        componentArray[newIndex] = component;
        ++size;
    }

    void removeData(Entity entity) {
        assert(entityToIndex.find(entity) != entityToIndex.end());
        size_t index = entityToIndex[entity];
        size_t lastIndex = size - 1;
        componentArray[index] = componentArray[lastIndex];

        Entity lastEntity = indexToEntity[lastIndex];
        entityToIndex[lastEntity] = index;
        indexToEntity[index] = lastEntity;

        entityToIndex.erase(entity);
        indexToEntity.erase(lastIndex);
        --size;
    }

    T& getData(Entity entity) {
        assert(entityToIndex.find(entity) != entityToIndex.end());
        return componentArray[entityToIndex[entity]];
    }

    void entityDestroyed(Entity entity) override {
        if (entityToIndex.find(entity) != entityToIndex.end())
            removeData(entity);
    }

private:
    std::array<T, MAX_ENTITIES> componentArray;
    std::unordered_map<Entity, size_t> entityToIndex;
    std::unordered_map<size_t, Entity> indexToEntity;
    size_t size = 0;
};

class ComponentManager {
public:
    template<typename T>
    void registerComponent() {
        const char* typeName = typeid(T).name();
        componentTypes[typeName] = nextComponentType;
        componentArrays[typeName] = std::make_shared<ComponentArray<T>>();
        ++nextComponentType;
    }

    template<typename T>
    ComponentType getComponentType() {
        const char* typeName = typeid(T).name();
        assert(componentTypes.find(typeName) != componentTypes.end());
        return componentTypes[typeName];
    }

    template<typename T>
    void addComponent(Entity entity, T component) {
        getComponentArray<T>()->insertData(entity, component);
    }

    template<typename T>
    void removeComponent(Entity entity) {
        getComponentArray<T>()->removeData(entity);
    }

    template<typename T>
    T& getComponent(Entity entity) {
        return getComponentArray<T>()->getData(entity);
    }

    void entityDestroyed(Entity entity) {
        for (auto const& pair : componentArrays) {
            auto const& component = pair.second;
            component->entityDestroyed(entity);
        }
    }

private:
    std::unordered_map<const char*, ComponentType> componentTypes{};
    std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays{};
    ComponentType nextComponentType = 0;

    template<typename T>
    std::shared_ptr<ComponentArray<T>> getComponentArray() {
        const char* typeName = typeid(T).name();
        assert(componentArrays.find(typeName) != componentArrays.end());
        return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeName]);
    }
};
