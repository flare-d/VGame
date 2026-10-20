#pragma once

// Total tank capacity. It is a plain timer expressed in abstract units:
// nothing in the game reads the absolute value, only its drain and refill.
constexpr float OXYGEN_MAX = 100.0f;

// Drain per second while fully submerged (SwimDive). At this rate the tank
// lasts about twelve seconds of diving, which fits the small pools of the
// current level; long underwater tunnels of the full game will gate on the
// tank upgrade instead of a bigger base value.
constexpr float OXYGEN_DRAIN_RATE = 8.0f; // per second while fully submerged

// Refill per second while swimming at the surface with the head above the
// water line: slower than dry land because the head barely clears the medium.
constexpr float OXYGEN_SURFACE_REGEN_RATE = 10.0f; // per second at the surface

// Refill per second on dry land or while wading shallow water.
constexpr float OXYGEN_REGEN_RATE = 30.0f; // per second out of the water

// Tank level granted right after a blackout damage event. A partial refill
// (not a full one) keeps the pressure on: diving straight back into the deep
// immediately after recovering is still a gamble.
constexpr float OXYGEN_AFTER_DAMAGE = 40.0f;
