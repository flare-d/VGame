#pragma once

// --- Damage stun ---
// A damage event never kills: the player is carried back to the last safe
// ground and stays immobilized for this long. The world keeps simulating
// during the stun; only the input is ignored.
constexpr float STUN_DURATION = 2.0f; // s of immobilization after a damage event

// --- Safe point tracking ---
// The spot under the player's feet becomes the damage respawn target only
// after this much uninterrupted safe standing. The delay filters out
// freshly landed frames at a ledge edge, so a damage event never drops the
// player back onto the lip of the pit they were escaping.
constexpr float SAFE_POINT_MIN_GROUND_TIME = 0.25f; // s of safe standing before the spot is recorded
