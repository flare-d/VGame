#pragma once

// --- Debug arrow drawing ---
// Force-to-pixel scale for the debug arrows: gravity (6000 N) draws as a
// 60 px arrow on the 320 px wide screen.
constexpr float DEBUG_FORCE_ARROW_SCALE = 0.01f;

// Upper bound of any single debug arrow length, in pixels. Longer vectors
// are shortened so strong forces stay readable on the small screen.
constexpr float DEBUG_ARROW_MAX_LEN = 60.0f;

// Velocity arrows use their own scale because velocity magnitudes (tens of
// px/s) differ wildly from force magnitudes (thousands of N).
constexpr float DEBUG_VELOCITY_ARROW_SCALE = 1.5f;

// --- Debug panel ---
// Width and height of the semi-transparent debug HUD anchored to the view
// corner. The height fits thirteen text lines: position, velocity,
// acceleration, state, ground, coyote, jump buffer, stun, oxygen, stamina,
// dash charge, contact flags, FPS and the camera position.
constexpr float DEBUG_PANEL_W = 130.0f;
constexpr float DEBUG_PANEL_H = 178.0f;

// Font size and vertical spacing of the debug panel lines.
constexpr unsigned DEBUG_FONT_SIZE = 8;
constexpr int DEBUG_LINE_STEP = 12;

// --- Pause overlay ---
// Font size of the centered PAUSED label drawn over the dimmed view.
constexpr unsigned PAUSE_FONT_SIZE = 16;
