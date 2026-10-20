#pragma once
#include "WorldParams.h"
#include "Wall.h"

// Size of the playable world in pixels. The camera is clamped so its view
// rectangle never leaves this area, which also keeps the player on screen.
constexpr int LEVEL_W = 960;
constexpr int LEVEL_H = 540;

// Y coordinate of the floor surface.
constexpr int GROUND_Y = 500;

// A body that falls past this line is out of the world: no wall can catch it
// anymore, so it is killed and respawned. It sits below the bottom edge of
// the floor slab (540) with a safety margin for one step of fall speed.
constexpr float KILL_Y = 620.0f;

// Player spawn position: standing on the floor near the left edge.
constexpr float PLAYER_SPAWN_X = 60.0f;
constexpr float PLAYER_SPAWN_Y = static_cast<float>(GROUND_Y - PLAYER_H);

// Static level geometry. Each entry is {x, y, w, h, type}; the type defaults
// to Solid. Platform tops rise by at most 44 px per step so every jump is
// possible with a held key.
inline constexpr Wall LEVEL_WALLS[] = {
    {0,   500, 960, 40},                          // floor spanning the whole level
    {100, 456, 56,  12},                          // step 1 (44 px rise from the floor)
    {220, 420, 56,  12},                          // step 2
    {340, 380, 56,  12},                          // step 3
    {460, 340, 64,  12},                          // step 4
    {580, 460, 16,  40},                          // low obstacle to hop over
    {660, 380, 56,  12},                          // right-side platform
    {780, 340, 56,  12},                          // high right platform
    {880, 420, 64,  12},                          // drop-down platform near the right edge
    {470, 220, 48,  16},                          // floating block high above step 4 (vertical camera test)
    {284, 484, 32,  16, WallType::BreakStomp},    // demo block: shatters when landed on from a jump
    {640, 484, 24,  16, WallType::BreakImpact},   // demo barrier: shatters only under a dash impact
};
inline constexpr unsigned LEVEL_WALL_COUNT =
    sizeof(LEVEL_WALLS) / sizeof(LEVEL_WALLS[0]);

// Plain data of one water volume: an axis-aligned box with no collision
// response of its own. The top edge (y) is the surface line; any body whose
// box overlaps the volume gets buoyancy and viscous drag. Named WaterZoneData
// so it can coexist with the WaterZone entity class name in future code
// without a redefinition error in translation units that see both.
struct WaterZoneData { float x, y, w, h; };

// Water volumes of the level. The first zone is a pool on the right floor,
// the second a shallow pond between the mid steps. Both are deep enough to
// cover the standing player (16 px tall), so both switch a grounded body
// into the swim states through the SWIM_ENTER_FRACTION depth test.
inline constexpr WaterZoneData WATER_ZONES[] = {
    {696, 468, 152, 32}, // pool between the right-side platforms
    {380, 484, 80,  16}, // shallow pond in front of step 4
};
inline constexpr unsigned WATER_ZONE_COUNT =
    sizeof(WATER_ZONES) / sizeof(WATER_ZONES[0]);
