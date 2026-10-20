#pragma once
#include "Entity.h"

// A water volume: an entity without collision whose rectangle defines the
// liquid region. The top edge of the rectangle is the surface line used by
// the buoyancy model; the zone never blocks movement (trigger semantics), so
// bodies enter and leave through its sides and surface freely.
class WaterZone : public Entity {
public:
    WaterZone(float x, float y, float w, float h) {
        pos = Vec2(x, y);
        shape.setSize({w, h});
        shape.setFillColor(sf::Color(40, 90, 200, 110)); // translucent water tint
        isTrigger = true; // volumes overlap without blocking movement
        layer = 0;
    }

    // Y coordinate of the surface line (the physics uses it as the depth
    // reference for the submerged-volume computation).
    float surfaceY() const { return pos.y; }
};

// Fraction of the body's height below the surface line, in the 0..1 range.
// The body is modelled as a vertical column, so the submerged volume grows
// linearly with depth; this fraction is the quantity the buoyancy force is
// proportional to. Keeping this a free function means any future body type
// (crates, actors) interacts with any water zone through the same formula.
inline float submergedFraction(const PhysicsBody& body, float surfaceY) {
    const float depth = (body.pos.y + body.h) - surfaceY;
    if (depth <= 0.0f)       return 0.0f;
    if (depth >= body.h)     return 1.0f;
    return depth / body.h;
}
