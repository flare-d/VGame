#pragma once

// Logical game resolution. The window is scaled up with letterboxing.
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 180;
constexpr int PLAYER_W = 16;
constexpr int PLAYER_H = 16;
constexpr int GROUND_Y = 160;

// --- Movement forces ---
constexpr float GRAVITY        = 1200.0f; // px/s^2, positive Y points down
constexpr float PLAYER_MASS    = 5.0f;    // kg
constexpr float MOVE_FORCE     = 3500.0f; // N, horizontal input force (ground and air)
constexpr float MAX_WALK_SPEED = 80.0f;   // px/s, ground speed cap in the input direction
constexpr float MAX_AIR_SPEED  = 90.0f;   // px/s, air speed cap in the input direction
constexpr float MAX_FALL_SPEED = 400.0f;  // px/s, hard terminal velocity clamp

// --- Ground friction ---
// FRICTION_STATIC must stay below MOVE_FORCE / normal force (3500/6000 ~= 0.58),
// otherwise static friction would cancel the input force and walking could never start.
constexpr float FRICTION_STATIC     = 0.40f; // mu_s
constexpr float FRICTION_KINETIC    = 0.35f; // mu_k
constexpr float FRICTION_STATIC_VEL = 2.0f;  // px/s, below this ground speed static friction may lock the body

// --- Air drag (quadratic, opposes velocity) ---
// Tuned so drag is a minor correction at walk speed and yields a soft terminal
// velocity near 620 px/s, while barely shortening jumps.
constexpr float AIR_DENSITY = 0.002f;
constexpr float DRAG_COEFF  = 1.0f;
constexpr float CROSS_AREA  = 16.0f; // px^2

// --- Jump ---
// The impulse alone gives a ~35 px apex (reaches the 32 px platform). Holding the
// key applies the boost force during ascent and extends the apex to ~54 px,
// enough to clear the 48 px platforms. Releasing the key early cuts the upward
// velocity, producing short hops.
constexpr float JUMP_IMPULSE     = 1450.0f; // kg*px/s, applied as an instant impulse
constexpr float JUMP_HOLD_FORCE  = 5000.0f; // N, extra upward force while the key is held during ascent
constexpr float MAX_JUMP_TIME    = 0.08f;   // s, longest duration of the hold boost
constexpr float JUMP_CUTOFF_MULT = 0.45f;   // early release scales upward velocity by this factor

// --- Timing windows ---
constexpr float COYOTE_TIME      = 0.10f; // s, a jump stays possible this long after leaving the ground
constexpr float JUMP_BUFFER_TIME = 0.10f; // s, a jump press is remembered this long while airborne
