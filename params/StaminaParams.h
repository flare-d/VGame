#pragma once

// Total grip reserve for wall climbing, in abstract units.
constexpr float STAMINA_MAX = 100.0f;

// Drain per second while actively climbing in a direction. At this rate the
// reserve lasts about four seconds of ascent, enough for roughly one and a
// half screen heights before the grip gives out.
constexpr float STAMINA_CLIMB_DRAIN = 25.0f; // per second while moving on the wall

// Drain per second while hanging still on the wall: cheaper than climbing,
// so pausing on a ledge to plan the next move is affordable.
constexpr float STAMINA_HANG_DRAIN = 10.0f; // per second while hanging still

// Refill per second in every state except wall climbing. Recovery is fast
// enough that a failed climb costs a short breather, not a detour.
constexpr float STAMINA_REGEN_RATE = 40.0f; // per second outside of climbing
