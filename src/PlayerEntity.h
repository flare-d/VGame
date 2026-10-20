#pragma once
#include "Entity.h"
#include "params/WorldParams.h"
#include "params/PlayerParams.h"

// The player character: a dynamic entity whose physics body is driven by
// input forces and jump impulses. Rendering tint (grounded vs airborne) is
// applied by the renderer every frame.
class PlayerEntity : public Entity {
public:
    explicit PlayerEntity(Vec2 spawnPos) {
        pos = spawnPos;
        body.emplace(PLAYER_MASS, spawnPos,
                     static_cast<float>(PLAYER_W), static_cast<float>(PLAYER_H));
        shape.setSize({static_cast<float>(PLAYER_W), static_cast<float>(PLAYER_H)});
        layer = 0;
    }
};
