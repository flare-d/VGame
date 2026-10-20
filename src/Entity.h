#pragma once
#include <optional>
#include <SFML/Graphics.hpp>
#include "Vec2.h"
#include "PhysicsBody.h"
#include "Wall.h"
#include "Animation.h"

// Base class for everything placed in a scene. An entity carries a position
// used as its rendering anchor, an optional physics body (present for dynamic
// objects, absent for static level geometry), a placeholder rectangle sprite,
// an animator driving its sprite-sheet rendering, a trigger flag and a
// collision layer index.
class Entity {
public:
    Vec2 pos{0.0f, 0.0f};               // rendering anchor / static collider origin
    std::optional<PhysicsBody> body;    // physics state; engaged only for dynamic entities
    sf::RectangleShape shape;           // placeholder sprite until real assets exist
    Animator animator;                  // active sprite-sheet animation, if any
    bool isTrigger = false;             // trigger volumes overlap without blocking movement
    bool isStaticCollider = false;      // static AABB geometry such as walls and platforms
    int layer = 0;                      // bodies only collide with geometry of the same layer

    // Static geometry only: how the wall responds to a hard contact, and
    // whether it has already been shattered. A broken wall stops colliding
    // and rendering until the world state is reloaded.
    WallType wallType = WallType::Solid;
    bool broken = false;

    Entity() = default;
    virtual ~Entity() = default;
};
