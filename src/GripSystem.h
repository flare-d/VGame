#pragma once
#include <memory>
#include <vector>
#include "Vec2.h"
#include "PhysicsBody.h"

// One available grip surface near the player. The normal points from the
// surface toward the player and defines both the push-away direction and the
// wall jump impulse direction.
struct GripInfo {
    bool valid = false;
    Vec2 normal;
};

// Source of grip surfaces. Each implementation detects its own kind of
// surface from the body state: walls today, boxes and vents can be added by
// registering another source without touching the climbing code.
class IGripSource {
public:
    virtual ~IGripSource() = default;
    virtual GripInfo probe(const PhysicsBody& body) const = 0;
};

// Detects a grippable wall from the contact flags filled by collision
// resolution. A wall on the player's right pushes back with normal (-1, 0).
class WallGripSource : public IGripSource {
public:
    GripInfo probe(const PhysicsBody& body) const override {
        if (body.touchingRightWall) return {true, Vec2(-1.0f, 0.0f)};
        if (body.touchingLeftWall)  return {true, Vec2(1.0f, 0.0f)};
        return GripInfo{};
    }
};

// Aggregates every registered grip source and returns the first surface the
// body can currently latch onto.
class GrabSystem {
public:
    void addSource(std::unique_ptr<IGripSource> source) {
        sources.push_back(std::move(source));
    }

    GripInfo findGrip(const PhysicsBody& body) const {
        for (const auto& s : sources) {
            const GripInfo g = s->probe(body);
            if (g.valid) return g;
        }
        return GripInfo{};
    }

private:
    std::vector<std::unique_ptr<IGripSource>> sources;
};
