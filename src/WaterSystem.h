#pragma once
#include <memory>
#include <vector>
#include "Vec2.h"
#include "params/WaterParams.h"
#include "params/LevelParams.h"

// Where the body is relative to the water right now. surfaceY is the world y
// of the surface line of the zone the body overlaps.
struct WaterInfo {
    bool valid = false;
    float surfaceY = 0.0f;
};

// Source of water volumes. Each implementation detects its own kind of water
// (static level pools today; moving tides or water volumes attached to moving
// platforms can be added by registering another source), so gameplay code
// never cares which kind of water it found.
class IWaterSource {
public:
    virtual ~IWaterSource() = default;
    virtual WaterInfo probe(float x, float y, float w, float h) const = 0;
};

// Detects overlap with the static water volumes loaded with the level. The
// query takes a plain box, so any body (the player now, crates later) goes
// through the same path.
class ZoneWaterSource : public IWaterSource {
public:
    WaterInfo probe(float x, float y, float w, float h) const override {
        for (unsigned i = 0; i < WATER_ZONE_COUNT; ++i) {
            const WaterZoneData& z = WATER_ZONES[i];
            if (x + w > z.x && x < z.x + z.w &&
                y + h > z.y && y < z.y + z.h)
                return {true, z.y};
        }
        return WaterInfo{};
    }
};

// Aggregates every registered water source and returns the first volume the
// box is currently inside.
class WaterSystem {
public:
    void addSource(std::unique_ptr<IWaterSource> source) {
        sources.push_back(std::move(source));
    }

    WaterInfo findWater(float x, float y, float w, float h) const {
        for (const auto& s : sources) {
            const WaterInfo info = s->probe(x, y, w, h);
            if (info.valid) return info;
        }
        return WaterInfo{};
    }

private:
    std::vector<std::unique_ptr<IWaterSource>> sources;
};

// Depth of the box below the surface line, clamped to the box height: zero
// when the box sits entirely above the water, equal to the height when fully
// submerged.
inline float submergedHeight(float boxTop, float boxH, float surfaceY) {
    const float d = (boxTop + boxH) - surfaceY;
    if (d < 0.0f)   return 0.0f;
    if (d > boxH)   return boxH;
    return d;
}

// Archimedean buoyancy of an axis-aligned box at the given submergence. Near
// the surface the magnitude follows the displaced volume linearly with depth,
// which makes the float stable and gives it the gentle rocking. Once the box
// is fully under, the magnitude is clamped to the body weight, so a submerged
// swimmer is neutrally buoyant and strokes rather than gravity govern the
// depth. Body-agnostic on purpose: crates and the player float or sink through
// this exact function.
inline Vec2 buoyancyForce(float mass, float boxW, float submerged, float boxH) {
    const float weight = mass * GRAVITY;
    float mag = LIQUID_DENSITY * boxW * submerged * GRAVITY;
    if (submerged >= boxH && mag > weight) mag = weight;
    return Vec2(0.0f, -mag);
}
