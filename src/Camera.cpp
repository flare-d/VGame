#include "Camera.h"
#include <algorithm>
#include <cmath>
#include "params/CameraParams.h"
#include "params/WorldParams.h"
#include "params/LevelParams.h"

namespace {

// Clamps a view center coordinate so the view rectangle of the given size
// never shows anything outside the level. A level smaller than the view is
// simply centered.
float clampCenter(float target, float levelSize, float viewSize) {
    if (levelSize <= viewSize) return levelSize / 2.0f;
    return std::clamp(target, viewSize / 2.0f, levelSize - viewSize / 2.0f);
}

} // namespace

void Camera::snapTo(const Vec2& focus) {
    pos.x = clampCenter(focus.x, static_cast<float>(LEVEL_W), static_cast<float>(SCREEN_W));
    pos.y = clampCenter(focus.y, static_cast<float>(LEVEL_H), static_cast<float>(SCREEN_H));
}

void Camera::update(float dt, const Vec2& focus) {
    // The view center can only reach a limited range of positions because of
    // the level clamp. Clamping the focus into that reachable range BEFORE
    // the deadzone test keeps the deadzone anchored to positions the camera
    // can actually take: without this, a player pushing past the level edge
    // leaves the reachable band and the deadzone target flips between the
    // clamped extremes every step, which reads as jitter at the borders.
    const float cx = clampCenter(focus.x, static_cast<float>(LEVEL_W), static_cast<float>(SCREEN_W));
    const float cy = clampCenter(focus.y, static_cast<float>(LEVEL_H), static_cast<float>(SCREEN_H));

    // Moves the desired view center only as far as needed to keep the focus
    // point inside the deadzone rectangle.
    Vec2 target = pos;
    const float dx = cx - pos.x;
    if (dx > CAM_DEADZONE_HALF_W)       target.x = cx - CAM_DEADZONE_HALF_W;
    else if (dx < -CAM_DEADZONE_HALF_W) target.x = cx + CAM_DEADZONE_HALF_W;
    const float dy = cy - pos.y;
    if (dy > CAM_DEADZONE_HALF_H)       target.y = cy - CAM_DEADZONE_HALF_H;
    else if (dy < -CAM_DEADZONE_HALF_H) target.y = cy + CAM_DEADZONE_HALF_H;

    // The view rectangle must stay fully inside the level, so the camera
    // starts moving before the player can reach the level edge.
    target.x = clampCenter(target.x, static_cast<float>(LEVEL_W), static_cast<float>(SCREEN_W));
    target.y = clampCenter(target.y, static_cast<float>(LEVEL_H), static_cast<float>(SCREEN_H));

    // Frame-rate independent exponential approach toward the target position.
    const float k = 1.0f - std::pow(1.0f - CAM_SMOOTH, dt * 60.0f);
    pos += (target - pos) * k;
}
