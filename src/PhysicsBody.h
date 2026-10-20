#pragma once
#include "Vec2.h"

// Dynamic physics state of one body. Forces accumulate during the frame via
// apply(), the game integrates them into velocity, then collision resolution
// moves the position and reports contact state for the next frame. The w/h
// pair is the collision box size; gameplay may swap it (the slide uses a low
// profile) and collision code always reads the current values.
struct PhysicsBody {
    Vec2 pos, vel, acc, force;
    float mass;
    float w = 16.0f, h = 16.0f; // collision box size in pixels

    bool onGround = false;

    // Contact state reported by collision resolution. Reset at the start of
    // every physics step and filled in during resolveCollisions; consumed by
    // gameplay code (wall climb, platform checks, button pressure).
    bool touchingFloor = false;
    bool touchingLeftWall = false;  // a wall is pressed against the left side
    bool touchingRightWall = false; // a wall is pressed against the right side
    Vec2 contactNormal;             // outward normal of the most recent contact

    // Member initializer order matches the declaration order above.
    PhysicsBody(float m, Vec2 p, float width, float height)
        : pos(p), mass(m), w(width), h(height) {}

    void reset() { force = Vec2(); acc = Vec2(); }
    void apply(const Vec2& f) { force += f; }
    void impulse(const Vec2& j) { vel += j / mass; }

    int px() const { return static_cast<int>(pos.x); }
    int py() const { return static_cast<int>(pos.y); }
};
