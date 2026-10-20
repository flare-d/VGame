#pragma once

// How a static wall reacts to a hard contact. Solid never breaks;
// BreakStomp shatters under a fast downward landing, BreakImpact under a
// fast sideways hit, BreakAny under either. The thresholds live in
// params/BreakableParams.h and are checked against the contact speed
// measured at the moment of the collision.
enum class WallType { Solid, BreakStomp, BreakImpact, BreakAny };

struct Wall {
    float x, y, w, h;
    WallType type = WallType::Solid; // walls initialized with four values stay solid
};
