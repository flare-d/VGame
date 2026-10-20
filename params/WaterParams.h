#pragma once
#include "PlayerParams.h"

// --- Medium properties ---
// Two-dimensional density: buoyancy equals density * submerged cross-section
// area * gravity. Tuned so the 16x16 player (5 kg) floats with half of its
// box underwater: 0.039 * (16 * 8) = 5 kg of displaced "water". A fully
// submerged box would displace about twice the body weight, which is why the
// buoyancy is clamped at the weight once the body is completely under.
constexpr float LIQUID_DENSITY = 0.039f; // kg/px^2

// Viscous linear damping of the medium, applied to both velocity components
// while the body is in water. This value also leaves the buoyancy spring
// underdamped, so a body dropped into the water rocks a few times around the
// equilibrium depth before settling instead of freezing in place.
constexpr float WATER_DRAG = 5.0f; // 1/s

// --- Swim entry depth ---
// Fraction of the body height that must be below the surface before a body
// standing on the floor is treated as swimming instead of grounded. A
// grounded body shallower than this is only wading: it keeps the Ground
// state, its jump and its friction, so puddles stay walkable. At 0.8 a body
// 80% under (12.8 px of the 16 px box) still wades, while the 16 px pond and
// the 32 px pool both trigger the swim states.
constexpr float SWIM_ENTER_FRACTION = 0.8f; // of body height

// --- Swim strokes ---
// Horizontal input swims along the surface or through the volume; vertical
// input strokes down (submerging from the surface, descending while diving)
// or up (surfacing). The cap is enforced by switching the stroke force off,
// never by clipping the velocity.
constexpr float SWIM_FORCE         = 1400.0f; // N, horizontal stroke
constexpr float SWIM_STROKE_FORCE  = 2200.0f; // N, vertical stroke
constexpr float SWIM_MAX_SPEED     = 70.0f;   // px/s, per-axis stroke speed cap

// --- Leaving the water ---
// A jump pressed at the surface converts into a straight upward impulse
// sized to reach banks standing roughly 30 px above the water line.
constexpr float SWIM_EXIT_BOOST = 1400.0f; // kg*px/s
