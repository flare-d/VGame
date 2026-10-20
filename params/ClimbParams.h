#pragma once

// --- Wall attachment ---
// While sliding or climbing, a steady press force pushes the body toward the
// wall. The wall normal force cancels the press during collision resolution,
// but the small per-frame penetration keeps refreshing the touching flags, so
// the grip probe stays valid and the state never flickers into Air. Player
// input directed away from the wall is far stronger than this press, so
// pulling off the wall still works.
constexpr float WALL_ATTACH_FORCE = 900.0f; // N, steady press toward the wall

// --- Wall sliding (pressing toward a wall while airborne, grab not held) ---
// The descent speed settles just above WALL_SLIDE_SPEED: each step, wall
// friction cancels only the velocity in excess of the target while gravity
// adds a small amount back, producing a steady, readable slide.
constexpr float WALL_SLIDE_SPEED        = 60.0f; // px/s target downward slide speed
constexpr float WALL_SLIDE_PUSH_IMPULSE = 30.0f; // kg*px/s, small away-from-wall kick on slide entry
constexpr float WALL_SLIDE_GRACE        = 0.08f; // s, slide persists this long after contact loss

// --- Wall climbing (grab held while touching a wall) ---
// The grip force cancels gravity so the body hangs without sinking, and the
// vertical input drives the body toward the climb speed through a damped
// force profile. No velocity is assigned directly anywhere.
constexpr float CLIMB_SPEED      = 60.0f; // px/s vertical speed while climbing
constexpr float CLIMB_DRIVE      = 12.0f; // 1/s, exponential approach rate toward climb speed
constexpr float WALL_GRIP_CANCEL = 1.0f;  // fraction of gravity cancelled by the wall grip

// --- Wall jump (jump pressed while climbing) ---
// The impulse points along the wall normal plus upward (180 px/s of vertical
// launch speed after the 5 kg mass divides it out). The control lock then
// ignores horizontal input toward the wall for a short window so the player
// cannot immediately re-stick to the same wall.
constexpr float WALL_JUMP_IMPULSE_X    = 260.0f; // kg*px/s along the wall normal
constexpr float WALL_JUMP_IMPULSE_Y    = 900.0f; // kg*px/s upward
constexpr float WALL_JUMP_CONTROL_LOCK = 0.10f;  // s
