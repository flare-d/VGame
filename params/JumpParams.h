#pragma once

// The impulse alone gives a ~35 px apex (reaches the 44 px step with margin on
// flat ground). Holding the key applies the boost force during ascent and
// extends the apex to ~54 px. Releasing the key early scales the upward
// velocity down, producing short hops.
constexpr float JUMP_IMPULSE     = 1450.0f; // kg*px/s, applied as an instant impulse
constexpr float JUMP_HOLD_FORCE  = 5000.0f; // N, extra upward force while the key is held during ascent
constexpr float MAX_JUMP_TIME    = 0.08f;   // s, longest duration of the hold boost
constexpr float JUMP_CUTOFF_MULT = 0.45f;   // early release scales upward velocity by this factor
