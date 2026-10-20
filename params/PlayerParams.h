#pragma once

// --- Core body properties ---
constexpr float GRAVITY     = 1200.0f; // px/s^2, positive Y points down
constexpr float PLAYER_MASS = 5.0f;    // kg

// --- Horizontal control ---
constexpr float MOVE_FORCE     = 3500.0f; // N, input force on the ground
constexpr float AIR_MOVE_FORCE = 2200.0f; // N, input force in air (reduced control)
constexpr float MAX_WALK_SPEED = 80.0f;   // px/s, ground speed cap in the input direction
constexpr float MAX_AIR_SPEED  = 90.0f;   // px/s, air speed cap in the input direction
constexpr float MAX_FALL_SPEED = 400.0f;  // px/s, hard terminal velocity clamp

// --- Ground friction ---
// Friction is modelled as pure braking: while the input force pushes in the
// direction of motion (or starts the body from rest) no kinetic friction acts,
// so acceleration stays responsive. When there is no driving input, or the input
// opposes the current velocity, FRICTION_BRAKE * normal force opposes the slide.
// The braking force is additionally capped per frame so it can stop the body
// exactly at zero but can never reverse the motion by itself (no jitter, no ice).
constexpr float FRICTION_BRAKE = 1.0f; // effective mu while braking

// --- Air friction ---
// Linear horizontal damping applied while airborne: the drag force equals
// -AIR_DRAG * mass * velocity_x, so free drift decays with a time constant
// of 1/AIR_DRAG seconds and cannot be felt as ice-sliding in the air.
constexpr float AIR_DRAG = 2.0f; // 1/s

// --- Quadratic air drag (opposes the full velocity vector) ---
// At gameplay speeds this term stays a minor correction. On a long vertical
// fall it would level out near 620 px/s, but the hard MAX_FALL_SPEED clamp
// cuts in earlier, so the clamp (400 px/s) is the effective terminal
// velocity of a fall and this term only shapes the approach to it.
constexpr float AIR_DENSITY = 0.002f;
constexpr float DRAG_COEFF  = 1.0f;
constexpr float CROSS_AREA  = 16.0f; // px^2
