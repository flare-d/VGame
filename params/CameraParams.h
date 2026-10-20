#pragma once

// Deadzone half-extents in world pixels. The camera holds still while the
// player center stays within this rectangle around the view center, and tracks
// the player only after the player crosses its edge.
constexpr float CAM_DEADZONE_HALF_W = 40.0f;
constexpr float CAM_DEADZONE_HALF_H = 32.0f;

// Smoothing factor per 1/60 s frame. Camera::update converts it to a
// frame-rate independent exponential approach rate.
constexpr float CAM_SMOOTH = 0.12f;
