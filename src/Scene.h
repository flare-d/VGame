#pragma once
#include <memory>
#include <vector>
#include <utility>
#include "Entity.h"

// Owns every entity of the game world. Entities are stored as unique pointers
// so derived types survive behind the Entity interface; the returned reference
// stays valid for the entity's whole lifetime.
class Scene {
public:
    // Constructs an entity of the given type inside the scene and returns it.
    template <typename T, typename... Args>
    T& spawn(Args&&... args) {
        auto entity = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *entity;
        entities.push_back(std::move(entity));
        return ref;
    }

    // Read-only access for rendering and probes.
    const std::vector<std::unique_ptr<Entity>>& getEntities() const { return entities; }

    // Mutable access for gameplay that changes entity state in place, such
    // as shattering a breakable wall during collision resolution.
    std::vector<std::unique_ptr<Entity>>& getEntities() { return entities; }

private:
    std::vector<std::unique_ptr<Entity>> entities;
};
