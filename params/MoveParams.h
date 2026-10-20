#pragma once

// --- Running (Shift held on the ground) ---
// The stronger input force raises the ground speed ceiling. The ceiling is
// enforced by switching the input force off at the cap, never by clipping
// the velocity, so momentum gained while running is preserved.
constexpr float RUN_FORCE     = 5200.0f; // N, horizontal input force while running
constexpr float MAX_RUN_SPEED = 130.0f;  // px/s, input-direction ceiling while running

// --- Run carry-over decay ---
// After Shift is released, speed above MAX_WALK_SPEED is left alone by the
// speed limiter; this linear drag acts only on the excess and bleeds it back
// to the walk cap with a time constant of 1/OVERSPEED_DRAG seconds, reading
// as rolling resistance plus air drag instead of a hard cut.
constexpr float OVERSPEED_DRAG = 3.0f; // 1/s, decay rate of speed beyond the walk cap

// --- Running jump ---
// The legs impulse keeps its full vertical magnitude, so the apex of a
// running leap equals the standing-jump apex; a horizontal term proportional
// to the current speed converts the run momentum into extra distance.
constexpr float RUN_JUMP_X_MULT    = 0.5f;   // fraction of current horizontal speed added as impulse
constexpr float RUN_JUMP_MIN_SPEED = 100.0f; // px/s, horizontal speed that counts as a running leap

// --- Slide (running speed + Down on the ground) ---
// The entry speed is preserved and bleeds away through the much smaller
// slide friction, giving a long braking path instead of a hard stop.
constexpr float SLIDE_MIN_SPEED  = 100.0f; // px/s, minimum entry speed
constexpr int   SLIDE_HEIGHT     = 8;      // px, low hitbox profile while sliding
constexpr float SLIDE_TIME       = 0.45f;  // s, longest slide duration
constexpr float SLIDE_FRICTION   = 0.12f;  // effective mu while sliding (ground braking uses 1.0)
constexpr float SLIDE_STOP_SPEED = 6.0f;   // px/s, a slower slide ends on its own

// --- Chain jumps ---
constexpr int   MAX_JUMPS       = 2;     // one ground jump plus one air jump per airtime
constexpr float AIR_JUMP_MULT   = 0.85f; // air jump impulse = JUMP_IMPULSE * this
// When true, an air jump starts the hold-boost phase exactly like a ground
// jump; when false it is a single plain impulse.
constexpr bool  AIR_JUMP_HOLD_BOOST = false;
