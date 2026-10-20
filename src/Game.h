#pragma once
#include <string>
#include <vector>
#include "Scene.h"
#include "PlayerEntity.h"
#include "Camera.h"
#include "GameState.h"
#include "GripSystem.h"
#include "WaterSystem.h"
#include "Vec2.h"
#include "SaveFile.h"
#include "params/MoveParams.h"
#include "params/DamageParams.h"
#include "params/OxygenParams.h"
#include "params/StaminaParams.h"
#include "params/DashParams.h"

// Generic press buffer: keeps a button press valid for a short window of time.
// If the matching action cannot run at the moment of the press, it can still
// trigger later while the window lasts. Used here for jump buffering.
struct ActionBuffer {
    float timer = 0.0f;
    void press(float duration) { timer = duration; }
    void tick(float dt) { timer -= dt; if (timer < 0.0f) timer = 0.0f; }
    bool ready() const { return timer > 0.0f; }
    void consume() { timer = 0.0f; }
};

// Locomotion states of the player body. Ground and Air reuse the base physics;
// WallSlide and WallClimb add wall-specific forces on top of it; Slide is the
// running crouch with its own low-friction braking; SwimSurface and SwimDive
// replace the medium with buoyancy and viscous drag; Dash is the burst move
// whose window overrides the integrated velocity with a fixed dash speed.
enum class MoveState {
    Ground,
    Air,
    WallSlide,
    WallClimb,
    Slide,
    SwimSurface,
    SwimDive,
    Dash
};

class Game {
    Scene scene;
    Entity* player = nullptr; // non-owning pointer to the player entity inside the scene
    Camera camera;
    GrabSystem grabSystem;
    WaterSystem waterSystem;

    // Static walls in spawn order, i.e. in the order of LEVEL_WALLS, so the
    // save file can reference them by plain indices.
    std::vector<Entity*> wallEntities;

    // Path of the save file; resolved by the caller relative to the
    // executable so the game behaves the same from any working directory.
    std::string savePath;

    GameState state = GameState::Playing;
    MoveState moveState = MoveState::Air;
    MoveState prevMoveState = MoveState::Air;

    // Direction of the most recent wall contact, kept one step so decisions
    // still work during the brief airborne gaps of a slide.
    Vec2 lastWallNormal{-1.0f, 0.0f};
    float wallSlideGrace = 0.0f;
    float wallJumpLock = 0.0f;

    float coyoteTimer = 0.0f;
    ActionBuffer jumpBuffer;
    bool jumpRequested = false;
    bool jumpActive = false;
    float jumpHoldTimer = 0.0f;

    // Chain-jump budget: restored to MAX_JUMPS while grounded, spent by jumps.
    int jumpsLeft = MAX_JUMPS;

    // Slide bookkeeping: the flag keeps the state alive, the timer caps it.
    bool sliding = false;
    float slideTimer = 0.0f;

    bool moveLeft = false, moveRight = false;
    bool moveUp = false, moveDown = false;
    bool holdJump = false;
    bool grabHeld = false;
    bool runHeld = false;
    bool dashRequested = false; // latched dash press, consumed by the dash trigger

    // --- Dash bookkeeping ---
    float dashTimer = 0.0f;    // remaining time of the velocity-override window
    bool dashAvailable = true; // the single air dash charge, refreshed by solid contacts
    Vec2 dashDir;              // normalized direction the current dash travels in
    int facing = 1;            // last horizontal input direction, +1 right / -1 left

    // --- Damage recovery ---
    // Counts down the immobilization after a damage event; while it is
    // positive every gameplay input reads as released.
    float stunTimer = 0.0f;
    // Feet position of the last spot where the player stood safely for a
    // continuous stretch; damage carries the body back to this point.
    Vec2 lastSafePos;
    // Continuous safe standing time; the safe point is recorded only after
    // it reaches SAFE_POINT_MIN_GROUND_TIME.
    float safeGroundTime = 0.0f;

    // --- Resources ---
    float oxygen = OXYGEN_MAX;   // tank content, drained by diving
    float stamina = STAMINA_MAX; // grip reserve, drained by climbing

    Vec2 dbgFin, dbgFg, dbgFf, dbgFd;

public:
    // saveFilePath anchors the snapshot file next to the executable.
    explicit Game(const std::string& saveFilePath = "savegame.json");
    // jumpReq and dashReq latch buffered presses; jumpHold, grab and run
    // track key states for the variable jump height, wall grip and running.
    void setInput(bool left, bool right, bool up, bool down,
                  bool jumpReq, bool jumpHold, bool grab, bool run,
                  bool dashReq);
    void update(float dt);

    // Carries the player back to the last safe ground, cancels every active
    // action and immobilizes the input for STUN_DURATION. Damage never
    // kills: hazards and an empty oxygen tank call this same entry point.
    void applyDamage();

    // Writes the current world state to the save file. Returns false when
    // the file cannot be written.
    bool saveGame();
    // Restores the world state from the save file. Returns false when no
    // compatible save exists, in which case the caller decides the fallback.
    bool loadGame();

    GameState getState() const { return state; }
    void setState(GameState s) { state = s; }

    const Scene& getScene() const { return scene; }
    const PhysicsBody& getPlayer() const { return *player->body; }
    const Camera& getCamera() const { return camera; }
    MoveState getMoveState() const { return moveState; }
    const char* getMoveStateName() const;

    Vec2 getDbgFin() const { return dbgFin; }
    Vec2 getDbgFg()  const { return dbgFg; }
    Vec2 getDbgFf()  const { return dbgFf; }
    Vec2 getDbgFd()  const { return dbgFd; }

    float getCoyoteTimer() const { return coyoteTimer; }
    float getJumpBufferTimer() const { return jumpBuffer.timer; }
    float getStunTimer() const { return stunTimer; }
    float getOxygen() const { return oxygen; }
    float getStamina() const { return stamina; }
    bool getDashAvailable() const { return dashAvailable; }

private:
    void resolveCollisions(float dt);
    // True when nothing solid blocks restoring the full-height hitbox above
    // the current low slide profile.
    bool hasStandRoom() const;
    // Grows or shrinks the hitbox toward the profile the current state needs,
    // keeping the feet anchored; growing is deferred until hasStandRoom().
    void refitHitbox();
    // Moves the player back to the spawn point and clears every piece of
    // gameplay state so the next step starts from a clean standing start.
    void respawn();
    // Copies the live player and world state into a plain snapshot struct.
    GameSnapshot captureSnapshot() const;
    // Overwrites the live state with a snapshot: position, resources, timers
    // and the broken flags of every wall the save file lists.
    void applySnapshot(const GameSnapshot& s);
};
