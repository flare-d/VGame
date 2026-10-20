#pragma once
#include "Vec2.h"

struct PhysicsBody {
    Vec2 pos, vel, acc, force;
    float mass;
    bool onGround = false;

    PhysicsBody(float m, Vec2 p) : mass(m), pos(p) {}

    void reset() { force = Vec2(); acc = Vec2(); }
    void apply(const Vec2& f) { force += f; }
    void impulse(const Vec2& j) { vel += j / mass; }

    int px() const { return static_cast<int>(pos.x); }
    int py() const { return static_cast<int>(pos.y); }
};
