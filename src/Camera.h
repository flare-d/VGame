#pragma once
#include "Vec2.h"

// Smooth-follow camera with a rectangular deadzone. The view center stays
// fixed while the focus point (the player center) remains inside the deadzone
// rectangle, tracks the focus once it crosses an edge, and is always clamped
// so the view rectangle stays fully inside the level.
class Camera {
    Vec2 pos; // world-space center of the view
public:
    // Places the view center directly on the clamped focus position.
    void snapTo(const Vec2& focus);

    // Advances the follow logic by dt seconds using the focus point.
    void update(float dt, const Vec2& focus);

    float x() const { return pos.x; }
    float y() const { return pos.y; }
};
