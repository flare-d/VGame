#pragma once

// --- Dash (the Celeste-style burst move) ---
// Fixed speed the dash overrides the velocity with for the whole dash
// window. It sits far above every run or wall-jump speed, which is what
// makes the dash the only move able to shatter Impact blocks and cross
// dash-gated gaps.
constexpr float DASH_SPEED = 260.0f; // px/s during the dash window

// How long the velocity override holds. Short enough to read as a burst,
// long enough to cross several tiles: 0.12 s at 260 px/s is ~31 px.
constexpr float DASH_TIME = 0.12f; // s

// Fraction of the dash velocity that survives the end of the window and
// carries into normal physics. A value below 1 keeps the dash a sharp
// burst instead of a permanent speed buff, but high enough to chain into
// a jump or a wall grab.
constexpr float DASH_END_SPEED_MULT = 0.75f;
