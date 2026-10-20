#include "Game.h"
#include <algorithm>
#include <cmath>
#include "params/PlayerParams.h"
#include "params/JumpParams.h"
#include "params/TimingParams.h"
#include "params/ClimbParams.h"
#include "params/MoveParams.h"
#include "params/WaterParams.h"
#include "params/BreakableParams.h"
#include "params/LevelParams.h"
#include "params/WorldParams.h"

namespace {

// The player's animation clip per locomotion state. Texture paths are
// relative to the assets directory; until the sheets exist the renderer
// falls back to the placeholder rectangles, so the pipeline works with an
// empty assets folder and picks the sheets up as soon as they appear.
const AnimationClip& clipForMoveState(MoveState s) {
    static const AnimationClip idle   {"player/idle.png",  16, 16, 2, 0.40f, true};
    static const AnimationClip run    {"player/run.png",   16, 16, 4, 0.10f, true};
    static const AnimationClip air    {"player/air.png",   16, 16, 1, 0.10f, true};
    static const AnimationClip slide  {"player/slide.png", 16, 8,  1, 0.10f, true};
    static const AnimationClip swim   {"player/swim.png",  16, 16, 4, 0.15f, true};
    static const AnimationClip climb  {"player/climb.png", 16, 16, 2, 0.20f, true};
    static const AnimationClip wall   {"player/wall.png",  16, 16, 1, 0.10f, true};
    static const AnimationClip dash   {"player/dash.png",  16, 16, 2, 0.05f, true};
    switch (s) {
        case MoveState::Ground:       return idle;
        case MoveState::Air:          return air;
        case MoveState::WallSlide:    return wall;
        case MoveState::WallClimb:    return climb;
        case MoveState::Slide:        return slide;
        case MoveState::SwimSurface:  return swim;
        case MoveState::SwimDive:     return swim;
        case MoveState::Dash:         return dash;
    }
    return idle;
}

} // namespace

Game::Game(const std::string& saveFilePath) : savePath(saveFilePath) {
    // Static level geometry becomes static collider entities on layer 0,
    // kept in a separate list so the save system can address them by index.
    for (unsigned i = 0; i < LEVEL_WALL_COUNT; ++i) {
        const Wall& w = LEVEL_WALLS[i];
        Entity& e = scene.spawn<Entity>();
        e.pos = Vec2(w.x, w.y);
        e.shape.setSize({w.w, w.h});
        e.shape.setFillColor(sf::Color(60, 90, 60));
        e.isStaticCollider = true;
        e.wallType = w.type;
        e.layer = 0;
        wallEntities.push_back(&e);
    }
    // The player is a dynamic entity owned by the scene; Game keeps a plain
    // pointer to drive it with input forces.
    player = &scene.spawn<PlayerEntity>(Vec2(PLAYER_SPAWN_X, PLAYER_SPAWN_Y));
    grabSystem.addSource(std::make_unique<WallGripSource>());
    waterSystem.addSource(std::make_unique<ZoneWaterSource>());
    jumpsLeft = MAX_JUMPS;
    // The spawn point is the initial damage respawn target.
    lastSafePos = Vec2(PLAYER_SPAWN_X, PLAYER_SPAWN_Y);
    camera.snapTo(Vec2(PLAYER_SPAWN_X + PLAYER_W * 0.5f, PLAYER_SPAWN_Y + PLAYER_H * 0.5f));
}

const char* Game::getMoveStateName() const {
    switch (moveState) {
        case MoveState::Ground:       return "Ground";
        case MoveState::Air:          return "Air";
        case MoveState::WallSlide:    return "WallSlide";
        case MoveState::WallClimb:    return "WallClimb";
        case MoveState::Slide:        return "Slide";
        case MoveState::SwimSurface:  return "SwimSurface";
        case MoveState::SwimDive:     return "SwimDive";
        case MoveState::Dash:         return "Dash";
    }
    return "Unknown";
}

void Game::setInput(bool left, bool right, bool up, bool down,
                    bool jumpReq, bool jumpHold, bool grab, bool run,
                    bool dashReq) {
    moveLeft = left;
    moveRight = right;
    moveUp = up;
    moveDown = down;
    if (jumpReq) jumpRequested = true; // latched until update() consumes it
    if (dashReq) dashRequested = true; // latched until the dash trigger consumes it
    holdJump = jumpHold;
    grabHeld = grab;
    runHeld = run;
}

bool Game::saveGame() {
    return writeSnapshot(savePath, captureSnapshot());
}

bool Game::loadGame() {
    GameSnapshot snapshot;
    if (!readSnapshot(savePath, snapshot)) return false;
    applySnapshot(snapshot);
    return true;
}

GameSnapshot Game::captureSnapshot() const {
    GameSnapshot s;
    const PhysicsBody& pb = *player->body;
    s.playerX = pb.pos.x;
    s.playerY = pb.pos.y;
    s.facing = facing;
    s.dashAvailable = dashAvailable;
    s.oxygen = oxygen;
    s.stamina = stamina;
    // Only shattered walls are stored: a wall missing from the list is
    // restored as solid, which is exactly the death-rollback semantics.
    for (unsigned i = 0; i < wallEntities.size(); ++i) {
        if (wallEntities[i]->broken)
            s.brokenWalls.push_back(static_cast<int>(i));
    }
    return s;
}

void Game::applySnapshot(const GameSnapshot& s) {
    PhysicsBody& pb = *player->body;

    // The body resumes at the saved spot with no momentum and no stale
    // contacts; the first steps after the load re-derive everything.
    pb.pos = Vec2(s.playerX, s.playerY);
    pb.vel = Vec2();
    pb.acc = Vec2();
    pb.force = Vec2();
    pb.onGround = false;
    pb.touchingFloor = false;
    pb.touchingLeftWall = false;
    pb.touchingRightWall = false;
    pb.contactNormal = Vec2();
    pb.h = static_cast<float>(PLAYER_H);

    facing = s.facing;
    dashAvailable = s.dashAvailable;
    oxygen = s.oxygen;
    stamina = s.stamina;

    // Every transient timer and action is cancelled; only the persistent
    // state carried by the snapshot survives.
    moveState = MoveState::Air;
    prevMoveState = MoveState::Air;
    lastWallNormal = Vec2(-1.0f, 0.0f);
    wallSlideGrace = 0.0f;
    wallJumpLock = 0.0f;
    coyoteTimer = 0.0f;
    jumpBuffer.timer = 0.0f;
    jumpRequested = false;
    jumpActive = false;
    jumpHoldTimer = 0.0f;
    jumpsLeft = MAX_JUMPS;
    sliding = false;
    slideTimer = 0.0f;
    dashTimer = 0.0f;
    stunTimer = 0.0f;
    lastSafePos = pb.pos;
    safeGroundTime = 0.0f;

    // Walls not listed as broken come back: a death undoes every shattered
    // block since the last save.
    for (unsigned i = 0; i < wallEntities.size(); ++i) {
        wallEntities[i]->broken =
            std::find(s.brokenWalls.begin(), s.brokenWalls.end(),
                      static_cast<int>(i)) != s.brokenWalls.end();
    }

    state = GameState::Playing;
    camera.snapTo(Vec2(pb.pos.x + pb.w * 0.5f, pb.pos.y + pb.h * 0.5f));
}

void Game::update(float dt) {
    // Frozen states run no simulation; rendering and input polling continue
    // in the main loop while gameplay is suspended.
    if (state != GameState::Playing) return;

    PhysicsBody& pb = *player->body;
    const bool wasOnGround = pb.onGround;
    pb.reset();
    pb.onGround = false;
    // Contact flags are recomputed from scratch every step by resolveCollisions.
    pb.touchingFloor = false;
    pb.touchingLeftWall = false;
    pb.touchingRightWall = false;
    pb.contactNormal = Vec2();
    dbgFin = dbgFg = dbgFf = dbgFd = Vec2();

    // --- Stun recovery ---
    // The timer counts down in real gameplay time; the world (gravity, water,
    // collisions) keeps simulating while the player is locked.
    if (stunTimer > 0.0f) stunTimer -= dt;
    const bool stunned = stunTimer > 0.0f;
    // Effective inputs: during the stun every action reads as released, so
    // the state machine, forces and jump logic all see an idle player while
    // the held key states stay untouched for the moment the stun ends.
    const bool inLeft     = moveLeft && !stunned;
    const bool inRight    = moveRight && !stunned;
    const bool inUp       = moveUp && !stunned;
    const bool inDown     = moveDown && !stunned;
    const bool inGrab     = grabHeld && !stunned;
    const bool inRun      = runHeld && !stunned;
    const bool inHoldJump = holdJump && !stunned;

    // --- Dash window upkeep ---
    // The window closes on its own; the body keeps a fraction of the dash
    // speed so the burst chains into normal physics instead of dead-stopping.
    if (dashTimer > 0.0f) {
        dashTimer -= dt;
        if (dashTimer <= 0.0f) {
            dashTimer = 0.0f;
            pb.vel *= DASH_END_SPEED_MULT;
        }
    }

    // --- Facing ---
    // The last unambiguous horizontal input direction feeds directionless
    // dashes, so tapping the dash key alone always bursts the way the
    // player was already heading.
    if (inLeft && !inRight) facing = -1;
    else if (inRight && !inLeft) facing = 1;

    // --- Jump budget: touching the ground restores the full jump chain ---
    if (wasOnGround) jumpsLeft = MAX_JUMPS;

    // --- Coyote time: grounded state stays jumpable briefly after walking off a ledge ---
    if (wasOnGround) coyoteTimer = COYOTE_TIME;
    else coyoteTimer -= dt;
    if (coyoteTimer < 0.0f) coyoteTimer = 0.0f;

    // --- Jump input buffer: a press is remembered and fires as soon as a jump becomes possible ---
    jumpBuffer.tick(dt);
    if (jumpRequested) {
        // A press landed during the stun is dropped instead of buffered, so
        // a buffered jump cannot fire inside the recovery window.
        if (stunned) jumpRequested = false;
        else jumpBuffer.press(JUMP_BUFFER_TIME);
    }
    jumpRequested = false;

    // --- Wall and slide timers ---
    if (wallJumpLock > 0.0f) wallJumpLock -= dt;
    if (wallSlideGrace > 0.0f) wallSlideGrace -= dt;
    if (sliding && slideTimer > 0.0f) slideTimer -= dt;

    // --- Grip probe from the contact flags of the previous step ---
    const GripInfo grip = grabSystem.findGrip(pb);
    if (grip.valid) lastWallNormal = grip.normal;
    const bool pressingTowardWall =
        (lastWallNormal.x < 0.0f && inRight) || (lastWallNormal.x > 0.0f && inLeft);

    // --- Water probe: which medium the body is in right now. The submerged
    // height drives both the state resolution and the buoyancy force. ---
    const WaterInfo water = waterSystem.findWater(pb.pos.x, pb.pos.y, pb.w, pb.h);
    const float submerged = water.valid
        ? submergedHeight(pb.pos.y, pb.h, water.surfaceY) : 0.0f;

    // --- Slide upkeep ---
    // The slide ends on its timer, when friction bled the entry speed away,
    // or when the ground disappears. Standing back up needs headroom; under
    // a low ceiling the slide simply continues until the space clears.
    if (sliding) {
        const bool wantsOut =
            slideTimer <= 0.0f || std::abs(pb.vel.x) < SLIDE_STOP_SPEED || !wasOnGround;
        if (!wantsOut || hasStandRoom()) sliding = false;
    }

    // --- Movement state resolution ---
    prevMoveState = moveState;
    // Depth test shared by the grounded and airborne branches: a body this
    // submerged (relative to its own height) is swimming even when the floor
    // is still within reach, so pools deep enough to cover the body win over
    // the grounded state. Shallower overlap only splashes, and a grounded
    // body keeps the Ground state, its friction and its ground jump.
    const bool deepInWater =
        water.valid && submerged >= pb.h * SWIM_ENTER_FRACTION;
    if (dashTimer > 0.0f) {
        // The dash window owns the body: no other state may claim it while
        // the velocity override is active.
        moveState = MoveState::Dash;
    } else if (sliding) {
        moveState = MoveState::Slide;
    } else if (wasOnGround && inDown && std::abs(pb.vel.x) >= SLIDE_MIN_SPEED) {
        // Running speed plus Down converts the momentum into a slide.
        sliding = true;
        slideTimer = SLIDE_TIME;
        moveState = MoveState::Slide;
    } else if (deepInWater) {
        // A body submerged deep enough to be covered by water leaves the
        // floor and swims: fully submerged bodies dive through the volume,
        // bodies still sticking out of the surface ride it. Running this
        // branch before the grounded branch is what lets the player walk
        // into a deep pool and start swimming instead of wading along its
        // bottom in the Ground state, where no buoyancy ever applied.
        moveState = (submerged >= pb.h) ? MoveState::SwimDive : MoveState::SwimSurface;
    } else if (wasOnGround) {
        moveState = MoveState::Ground;
    } else if (water.valid && submerged > 0.0f) {
        // Airborne entry into shallow water (a jump landing in a puddle):
        // any submersion starts the swim right away, matching how a falling
        // body plops into the medium instead of waiting to reach the floor.
        moveState = (submerged >= pb.h) ? MoveState::SwimDive : MoveState::SwimSurface;
    } else if (inGrab && grip.valid && stamina > 0.0f) {
        // An exhausted grip (empty stamina) cannot latch onto the wall: the
        // branch demands a non-zero reserve, so the player falls and the
        // stamina refills before the next attempt.
        moveState = MoveState::WallClimb;
    } else if (wallJumpLock <= 0.0f && pressingTowardWall &&
               (grip.valid || wallSlideGrace > 0.0f)) {
        moveState = MoveState::WallSlide;
    } else {
        moveState = MoveState::Air;
    }
    // A slide in full contact arms the grace window; it keeps the slide alive
    // through the micro-separations caused by the push-away kick.
    if (moveState == MoveState::WallSlide && grip.valid)
        wallSlideGrace = WALL_SLIDE_GRACE;

    const bool swimming = moveState == MoveState::SwimSurface ||
                           moveState == MoveState::SwimDive;

    // --- Dash trigger ---
    // The latched press fires while the charge is available, the player is
    // not stunned and not swimming: water has its own movement verbs. Any
    // other state (ground, air, slide, climb, wall slide) converts straight
    // into the burst.
    if (dashRequested) {
        dashRequested = false;
        if (!stunned && dashAvailable && !swimming) {
            Vec2 dir(0.0f, 0.0f);
            if (inLeft)  dir.x -= 1.0f;
            if (inRight) dir.x += 1.0f;
            if (inUp)    dir.y -= 1.0f;
            if (inDown)  dir.y += 1.0f;
            // No direction held: burst toward where the player was heading.
            if (dir.lenSq() < 0.01f) dir = Vec2(static_cast<float>(facing), 0.0f);
            dashDir = dir.norm();
            dashTimer = DASH_TIME;
            dashAvailable = false;
            // The dash cancels every action it overrides: no slide profile,
            // no jump boost, no climb hangover.
            jumpActive = false;
            sliding = false;
            slideTimer = 0.0f;
            moveState = MoveState::Dash;
        }
    }

    // --- Hitbox refit toward the profile the current state needs ---
    refitHitbox();

    // --- Animation ---
    // The animator holds the clip matching the current locomotion state and
    // advances it with gameplay time. Rendering turns the frame index into a
    // texture rectangle, or falls back to the placeholder rectangle while
    // the sheet for the clip does not exist yet.
    player->animator.play(clipForMoveState(moveState));
    player->animator.update(dt);

    // --- Oxygen: diving drains the tank, dry air and the surface refill it ---
    if (moveState == MoveState::SwimDive) {
        oxygen -= OXYGEN_DRAIN_RATE * dt;
    } else if (swimming) {
        oxygen += OXYGEN_SURFACE_REGEN_RATE * dt; // head above the water line
    } else {
        oxygen += OXYGEN_REGEN_RATE * dt;         // dry land refills fastest
    }
    if (oxygen <= 0.0f) {
        // A blackout is a damage event, never death: the player is carried
        // back to the last safe ground, and the tank refills only partially
        // so diving straight back into the deep stays a gamble.
        oxygen = OXYGEN_AFTER_DAMAGE;
        applyDamage();
    }
    oxygen = std::clamp(oxygen, 0.0f, OXYGEN_MAX);

    // --- Stamina: climbing spends the grip reserve, everything else restores it ---
    if (moveState == MoveState::WallClimb) {
        const bool climbingMoving = inUp || inDown;
        stamina -= (climbingMoving ? STAMINA_CLIMB_DRAIN : STAMINA_HANG_DRAIN) * dt;
        if (stamina < 0.0f) stamina = 0.0f; // the empty grip releases on the next step
    } else {
        stamina += STAMINA_REGEN_RATE * dt;
    }
    stamina = std::clamp(stamina, 0.0f, STAMINA_MAX);

    // 1. GRAVITY
    // Gravity always acts; in water the buoyancy below answers it, and once
    // the body is fully submerged the clamped buoyancy exactly replaces it.
    // During the dash window the force still accumulates (the debug arrows
    // stay honest) but the velocity override at integration discards it.
    Vec2 Fg(0.0f, pb.mass * GRAVITY);
    pb.apply(Fg);
    dbgFg = Fg;

    // 2. MEDIUM RESISTANCE
    if (swimming) {
        // Viscous water drag opposes both velocity components; together with
        // the buoyancy this is what kills a falling body's speed and lets the
        // strokes dominate the motion inside the volume.
        Vec2 Fw(-pb.vel.x * pb.mass * WATER_DRAG,
                -pb.vel.y * pb.mass * WATER_DRAG);
        pb.apply(Fw);
        dbgFd = Fw;
    } else if (pb.vel.lenSq() > 1e-4f) {
        // Quadratic air drag (opposes the full velocity vector).
        const float v = pb.vel.len();
        const float mag = 0.5f * AIR_DENSITY * v * v * DRAG_COEFF * CROSS_AREA;
        Vec2 Fd = pb.vel.norm() * (-mag);
        pb.apply(Fd);
        dbgFd = Fd;
    }

    // 3. PLANNED INPUT FORCE (running is stronger, air control is weaker)
    const float inputMag = wasOnGround ? (inRun ? RUN_FORCE : MOVE_FORCE)
                                       : AIR_MOVE_FORCE;
    Vec2 Finput(0.0f, 0.0f);
    if (inLeft)  Finput.x -= inputMag;
    if (inRight) Finput.x += inputMag;
    const float rawInputX = Finput.x; // kept for friction decisions below

    // Horizontal input pushing INTO a wall is cancelled by the wall normal
    // force. The same rule applies during the wall jump control lock, so the
    // player cannot accelerate back toward the wall right after jumping off.
    const bool lockedTowardWall = wallJumpLock > 0.0f && (
        (lastWallNormal.x < 0.0f && Finput.x > 0.0f) ||
        (lastWallNormal.x > 0.0f && Finput.x < 0.0f));
    const bool pushingIntoWall =
        (moveState == MoveState::WallSlide || moveState == MoveState::WallClimb) && (
        (lastWallNormal.x < 0.0f && Finput.x > 0.0f) ||
        (lastWallNormal.x > 0.0f && Finput.x < 0.0f));
    if (lockedTowardWall || pushingIntoWall) Finput.x = 0.0f;

    // Steering is disabled during the ground slide, the dash (the dash
    // direction was chosen at the trigger) and the swim strokes, which are
    // applied by the water branch below.
    if (moveState == MoveState::Slide || moveState == MoveState::Dash || swimming)
        Finput.x = 0.0f;

    // 4. SURFACE FORCES
    if (moveState == MoveState::Ground) {
        // The input "drives" the motion when it pushes along the current
        // velocity or starts the body from rest. In that regime no kinetic
        // friction acts and acceleration stays crisp.
        const bool inputDrivesMotion =
            rawInputX != 0.0f &&
            (std::abs(pb.vel.x) <= 1.0f || sign(rawInputX) == sign(pb.vel.x));
        const bool overSpeedBleed =
            inputDrivesMotion && !inRun && std::abs(pb.vel.x) > MAX_WALK_SPEED;
        if (overSpeedBleed) {
            // Carry-over from a released run: a linear drag acts only on the
            // speed in excess of the walk cap and decays it back to
            // MAX_WALK_SPEED smoothly instead of cutting it in one frame.
            const float excess =
                pb.vel.x - std::clamp(pb.vel.x, -MAX_WALK_SPEED, MAX_WALK_SPEED);
            Vec2 Fo(-pb.mass * OVERSPEED_DRAG * excess, 0.0f);
            pb.apply(Fo);
            dbgFf = Fo;
        } else if (!inputDrivesMotion && std::abs(pb.vel.x) > 0.0f) {
            // Braking regime: strong friction opposes the slide, capped so it
            // can stop the body within this frame but never reverse it.
            const float normalForce = pb.mass * GRAVITY;
            float fMag = FRICTION_BRAKE * normalForce;
            const float stopForce = std::abs(pb.vel.x) * pb.mass / dt;
            if (fMag > stopForce) fMag = stopForce;
            Vec2 Ff(-sign(pb.vel.x) * fMag, 0.0f);
            pb.apply(Ff);
            dbgFf = Ff;
        }
    } else if (moveState == MoveState::Slide) {
        // The same capped-braking model as the ground, but with a much
        // smaller effective mu: the entry speed bleeds away over a long
        // distance instead of stopping on the spot.
        if (std::abs(pb.vel.x) > 0.0f) {
            const float normalForce = pb.mass * GRAVITY;
            float fMag = SLIDE_FRICTION * normalForce;
            const float stopForce = std::abs(pb.vel.x) * pb.mass / dt;
            if (fMag > stopForce) fMag = stopForce;
            Vec2 Ff(-sign(pb.vel.x) * fMag, 0.0f);
            pb.apply(Ff);
            dbgFf = Ff;
        }
    } else if (swimming) {
        // Buoyancy from the submerged part of the box. Near the surface the
        // magnitude follows the displaced volume, so the float settles where
        // buoyancy balances gravity and rocks around that depth; fully
        // submerged the clamp keeps the swimmer neutrally buoyant.
        const Vec2 Fb = buoyancyForce(pb.mass, pb.w, submerged, pb.h);
        pb.apply(Fb);
        dbgFf = Fb; // the yellow debug arrow shows the buoyancy

        // Swim strokes: horizontal input rows along the current direction,
        // vertical input strokes down (submerging from the surface,
        // descending while diving) or up (surfacing). The cap is enforced by
        // switching the stroke force off, never by clipping the velocity.
        Vec2 Fs(0.0f, 0.0f);
        if (inLeft)      Fs.x -= SWIM_FORCE;
        if (inRight)     Fs.x += SWIM_FORCE;
        if (inUp)        Fs.y -= SWIM_STROKE_FORCE;
        else if (inDown) Fs.y += SWIM_STROKE_FORCE;
        if (Fs.x > 0.0f && pb.vel.x >=  SWIM_MAX_SPEED) Fs.x = 0.0f;
        if (Fs.x < 0.0f && pb.vel.x <= -SWIM_MAX_SPEED) Fs.x = 0.0f;
        if (Fs.y > 0.0f && pb.vel.y >=  SWIM_MAX_SPEED) Fs.y = 0.0f;
        if (Fs.y < 0.0f && pb.vel.y <= -SWIM_MAX_SPEED) Fs.y = 0.0f;
        pb.apply(Fs);
        dbgFin = Fs; // swim strokes take the input-force debug arrow
    } else if (moveState == MoveState::WallSlide) {
        // Kinetic wall friction cancels only the velocity in excess of the
        // slide target. Gravity keeps feeding speed in, so the descent settles
        // into a steady slide just above WALL_SLIDE_SPEED.
        if (pb.vel.y > WALL_SLIDE_SPEED) {
            Vec2 Fs(0.0f, -pb.mass * (pb.vel.y - WALL_SLIDE_SPEED) / dt);
            pb.apply(Fs);
            dbgFf = Fs;
        }
        // A steady press toward the wall keeps the contact alive: the wall
        // normal force cancels the press during collision resolution, but the
        // small per-frame penetration refreshes the touching flag, so the grip
        // probe never loses the wall mid-slide.
        pb.apply(Vec2(-lastWallNormal.x * WALL_ATTACH_FORCE, 0.0f));
        // A small away-from-wall kick on entry makes the slide readable.
        if (prevMoveState != MoveState::WallSlide)
            pb.impulse(Vec2(lastWallNormal.x * WALL_SLIDE_PUSH_IMPULSE, 0.0f));
    } else if (moveState == MoveState::WallClimb) {
        // The grip force cancels gravity, so a grabbed body hangs still.
        pb.apply(Vec2(0.0f, -pb.mass * GRAVITY * WALL_GRIP_CANCEL));
        // Vertical input drives the body toward the climb speed through a
        // damped force profile: F = m * k * (target - v). The exponential
        // approach never overshoots and needs no direct velocity assignment.
        float targetVy = 0.0f;
        if (inUp)        targetVy = -CLIMB_SPEED;
        else if (inDown) targetVy =  CLIMB_SPEED;
        Vec2 Fc(0.0f, pb.mass * CLIMB_DRIVE * (targetVy - pb.vel.y));
        pb.apply(Fc);
        dbgFf = Fc; // the yellow debug arrow shows the climb drive force
        // The same steady press as in the slide keeps the body glued to the
        // wall even when the player is not pushing toward it, so hanging in
        // place stays a stable WallClimb instead of flickering into Air.
        pb.apply(Vec2(-lastWallNormal.x * WALL_ATTACH_FORCE, 0.0f));
    } else {
        // Air friction: linear damping of horizontal drift so airborne momentum
        // decays instead of carrying the player like a hockey puck. The Dash
        // state lands here too: the force accumulates for the arrows, and the
        // velocity override at integration does the real work.
        Vec2 Fa(-pb.vel.x * pb.mass * AIR_DRAG, 0.0f);
        pb.apply(Fa);
        dbgFf = Fa;
    }

    // 5. INPUT-DIRECTION SPEED CEILING
    // The input force switches off at the cap instead of clipping the
    // velocity after integration, so momentum from a run carry-over, a slide
    // exit or a wall jump is never cut; only friction and drag erode it.
    {
        const float cap = wasOnGround ? (inRun ? MAX_RUN_SPEED : MAX_WALK_SPEED)
                                      : MAX_AIR_SPEED;
        if (Finput.x > 0.0f && pb.vel.x >= cap) Finput.x = 0.0f;
        else if (Finput.x < 0.0f && pb.vel.x <= -cap) Finput.x = 0.0f;
    }

    pb.apply(Finput);
    if (!swimming) dbgFin = Finput; // swim strokes reported their own value

    // 6. JUMP EXECUTION
    // A buffered press while climbing fires a wall jump: an impulse along the
    // wall normal plus upward, followed by the control lock window.
    if (jumpBuffer.ready() && moveState == MoveState::WallClimb) {
        pb.impulse(Vec2(lastWallNormal.x * WALL_JUMP_IMPULSE_X, -WALL_JUMP_IMPULSE_Y));
        wallJumpLock = WALL_JUMP_CONTROL_LOCK;
        jumpActive = false;
        dashTimer = 0.0f; // the wall jump cancels an active dash
        moveState = MoveState::Air;
        coyoteTimer = 0.0f;
        jumpBuffer.consume();
    } else if (jumpBuffer.ready() && moveState == MoveState::SwimSurface) {
        // A jump at the surface converts into a straight upward impulse sized
        // to climb out onto banks a bit above the water line. The jump budget
        // is untouched, so the air jump remains available after the landing.
        pb.impulse(Vec2(0.0f, -SWIM_EXIT_BOOST));
        jumpActive = false;
        dashTimer = 0.0f;
        coyoteTimer = 0.0f;
        jumpBuffer.consume();
    } else if (jumpBuffer.ready() && coyoteTimer > 0.0f && jumpsLeft > 0) {
        // Ground (or coyote) jump. The vertical impulse is always the full
        // JUMP_IMPULSE; a horizontal term proportional to the current speed
        // is added during a running leap, so the apex is unchanged and only
        // the distance grows.
        Vec2 j(0.0f, -JUMP_IMPULSE);
        if (inRun && std::abs(pb.vel.x) >= RUN_JUMP_MIN_SPEED)
            j.x = pb.mass * RUN_JUMP_X_MULT * pb.vel.x;
        pb.impulse(j);
        jumpActive = true;
        jumpHoldTimer = 0.0f;
        jumpBuffer.consume();
        coyoteTimer = 0.0f;
        jumpsLeft -= 1;
        dashTimer = 0.0f; // jumping out of a dash ends the burst
        // A jump fired from a running slide ends the slide immediately, so
        // the hitbox refit of the next step restores the full standing
        // profile instead of carrying the low crouch into the air.
        sliding = false;
        slideTimer = 0.0f;
    } else if (jumpBuffer.ready() && jumpsLeft > 1) {
        // Chain jump: it requires the ground slot to be still unused and
        // consumes the whole remaining budget at once, so walking off a ledge
        // without jumping never grants more than a single air jump. It does
        // not touch coyote time and starts the hold-boost phase only when
        // AIR_JUMP_HOLD_BOOST says so.
        pb.impulse(Vec2(0.0f, -JUMP_IMPULSE * AIR_JUMP_MULT));
        jumpBuffer.consume();
        coyoteTimer = 0.0f;
        jumpsLeft = 0;
        jumpActive = AIR_JUMP_HOLD_BOOST;
        jumpHoldTimer = 0.0f;
        dashTimer = 0.0f;
    }

    // 7. VARIABLE JUMP HEIGHT
    // The boost phase ends at the apex of the trajectory: the jump impulse
    // points up, so the velocity turns positive exactly at the top. Landing
    // contact is reported only later, by resolveCollisions(), and is handled
    // there; watching the velocity sign here keeps this check honest without
    // reading a flag that was already reset at the top of this step.
    if (jumpActive && pb.vel.y >= 0.0f) jumpActive = false;
    if (jumpActive && inHoldJump) {
        jumpHoldTimer += dt;
        if (jumpHoldTimer < MAX_JUMP_TIME) {
            pb.apply(Vec2(0.0f, -JUMP_HOLD_FORCE));
        } else {
            jumpActive = false;
        }
    } else if (jumpActive && !inHoldJump) {
        // Early release: cut the upward velocity to a fraction of its value.
        jumpActive = false;
        if (pb.vel.y < 0.0f) pb.vel.y *= JUMP_CUTOFF_MULT;
    }

    // 8. INTEGRATE ACCELERATION INTO VELOCITY
    pb.acc = pb.force / pb.mass;
    pb.vel += pb.acc * dt;
    if (dashTimer > 0.0f) {
        // The dash window owns the velocity: the accumulated forces (gravity,
        // drag, input) are discarded and the body travels at the fixed dash
        // speed in the chosen direction instead.
        pb.vel = dashDir * DASH_SPEED;
    }

    // Hard fall speed clamp: guarantees stable collisions regardless of tuning.
    if (pb.vel.y > MAX_FALL_SPEED) pb.vel.y = MAX_FALL_SPEED;

    // 9. AXIS-SEPARATED COLLISION RESOLUTION
    resolveCollisions(dt);

    // A dash that ran into a wall ends where the wall stopped it: the
    // momentum died in the collision, so keeping the window alive would only
    // press the body against the face.
    if (dashTimer > 0.0f && (
        (pb.touchingLeftWall && dashDir.x < 0.0f) ||
        (pb.touchingRightWall && dashDir.x > 0.0f))) {
        dashTimer = 0.0f;
    }

    // Solid ground and grippable walls refresh the single air dash charge.
    if (pb.touchingFloor || grip.valid) dashAvailable = true;

    // A landing detected by the resolution above cancels the hold boost on
    // the exact contact frame, before the boost could keep pulling a body
    // that already stands on the floor.
    if (jumpActive && pb.touchingFloor) jumpActive = false;

    // --- Safe point bookkeeping ---
    // Standing on solid ground outside the swim states counts as safe
    // territory; the timer demands an uninterrupted stretch so a single
    // grounded frame at a ledge edge never becomes the damage respawn target.
    if (pb.touchingFloor && !swimming) safeGroundTime += dt;
    else safeGroundTime = 0.0f;
    if (safeGroundTime >= SAFE_POINT_MIN_GROUND_TIME) lastSafePos = pb.pos;

    // 10. CAMERA FOLLOW (focus point: player center)
    camera.update(dt, Vec2(pb.pos.x + pb.w * 0.5f, pb.pos.y + pb.h * 0.5f));

    // Kill plane: a body below every possible floor is out of the world, and
    // a death rolls the whole game back to the last saved snapshot. With no
    // compatible save on disk the game falls back to a fresh start at the
    // spawn point.
    if (pb.pos.y > KILL_Y) {
        state = GameState::Dead;
        if (!loadGame()) respawn();
    }
}

void Game::applyDamage() {
    // A damage event during the recovery window is ignored: repeated hits
    // cannot chain-stun the player before the first stun even ends.
    if (stunTimer > 0.0f) return;

    PhysicsBody& pb = *player->body;

    // The body is carried back to the last safe ground with every trace of
    // motion removed; the world itself is untouched, because damage is a
    // soft reset of the player, never of the level.
    pb.pos = lastSafePos;
    pb.vel = Vec2();
    pb.acc = Vec2();
    pb.force = Vec2();
    pb.contactNormal = Vec2();
    // Safe points are recorded standing on open ground, so the full-height
    // hitbox is restored unconditionally (a hit mid-slide must not leave
    // the low profile behind).
    pb.h = static_cast<float>(PLAYER_H);

    // Every active action is cancelled: no dash, no slide, no jump boost,
    // no wall lock, and the buffered press is spent.
    moveState = MoveState::Air; // resolves to Ground on the next step
    prevMoveState = MoveState::Air;
    sliding = false;
    slideTimer = 0.0f;
    dashTimer = 0.0f;
    wallJumpLock = 0.0f;
    wallSlideGrace = 0.0f;
    coyoteTimer = 0.0f;
    jumpBuffer.timer = 0.0f;
    jumpRequested = false;
    jumpActive = false;
    jumpHoldTimer = 0.0f;

    stunTimer = STUN_DURATION;
}

void Game::respawn() {
    PhysicsBody& pb = *player->body;

    // The body restarts at the spawn point with every accumulator zeroed:
    // no momentum, no leftover forces, no stale contacts.
    pb.pos = Vec2(PLAYER_SPAWN_X, PLAYER_SPAWN_Y);
    pb.vel = Vec2();
    pb.acc = Vec2();
    pb.force = Vec2();
    pb.onGround = false;
    pb.touchingFloor = false;
    pb.touchingLeftWall = false;
    pb.touchingRightWall = false;
    pb.contactNormal = Vec2();
    // A death can happen mid-slide with the low hitbox; the full standing
    // profile is restored unconditionally because the spawn point is clear.
    pb.h = static_cast<float>(PLAYER_H);

    // Every wall returns: a fresh start means an untouched world.
    for (Entity* e : wallEntities) e->broken = false;

    // Every movement aid and buffer resets so the first step after the
    // respawn behaves exactly like the first step of a fresh game.
    moveState = MoveState::Air;
    prevMoveState = MoveState::Air;
    lastWallNormal = Vec2(-1.0f, 0.0f);
    wallSlideGrace = 0.0f;
    wallJumpLock = 0.0f;
    coyoteTimer = 0.0f;
    jumpBuffer.timer = 0.0f;
    jumpRequested = false;
    jumpActive = false;
    jumpHoldTimer = 0.0f;
    jumpsLeft = MAX_JUMPS;
    sliding = false;
    slideTimer = 0.0f;
    dashTimer = 0.0f;
    dashAvailable = true;
    facing = 1;

    // A kill is a death, not a damage event: no stun and no safe-point
    // teleport, the game simply starts over from the spawn point.
    stunTimer = 0.0f;
    lastSafePos = Vec2(PLAYER_SPAWN_X, PLAYER_SPAWN_Y);
    safeGroundTime = 0.0f;
    oxygen = OXYGEN_MAX;
    stamina = STAMINA_MAX;

    state = GameState::Playing;
    camera.snapTo(Vec2(PLAYER_SPAWN_X + PLAYER_W * 0.5f,
                       PLAYER_SPAWN_Y + PLAYER_H * 0.5f));
}

void Game::resolveCollisions(float dt) {
    PhysicsBody& pb = *player->body;

    // --- X axis ---
    float newX = pb.pos.x + pb.vel.x * dt;
    for (auto& e : scene.getEntities()) {
        // Only solid static geometry of the player's own layer can block movement.
        if (!e->isStaticCollider || e->isTrigger) continue;
        if (e->layer != player->layer) continue;
        if (e->broken) continue; // shattered walls no longer exist for physics
        const sf::Vector2f sz = e->shape.getSize();
        if (newX + pb.w > e->pos.x && newX < e->pos.x + sz.x &&
            pb.pos.y + pb.h > e->pos.y && pb.pos.y < e->pos.y + sz.y) {
            // A hard sideways hit shatters the matching breakable walls
            // BEFORE the resolution: the wall disappears and the body keeps
            // its speed, which is what makes a dash-through read as a smash
            // instead of a stop.
            if ((e->wallType == WallType::BreakImpact || e->wallType == WallType::BreakAny) &&
                std::abs(pb.vel.x) >= BREAK_IMPACT_MIN_SPEED) {
                e->broken = true;
                continue;
            }
            if (pb.vel.x > 0.0f) {
                newX = e->pos.x - pb.w;
                pb.touchingRightWall = true;
                pb.contactNormal += Vec2(-1.0f, 0.0f);
            } else if (pb.vel.x < 0.0f) {
                newX = e->pos.x + sz.x;
                pb.touchingLeftWall = true;
                pb.contactNormal += Vec2(1.0f, 0.0f);
            } else {
                // The body does not move horizontally this step but its box
                // still overlaps the wall (for example right after a hitbox
                // refit under a ceiling squashed it into the geometry): push
                // it out along the side of least penetration so a body can
                // never come to rest inside solid geometry.
                const float penFromLeftFace  = newX + pb.w - e->pos.x;        // depth past the wall's left edge
                const float penFromRightFace = e->pos.x + sz.x - newX;        // depth past the wall's right edge
                if (penFromLeftFace < penFromRightFace) {
                    newX = e->pos.x - pb.w;       // exits to the left of the wall
                    pb.touchingRightWall = true;  // the wall face now touches the body's right side
                    pb.contactNormal += Vec2(-1.0f, 0.0f);
                } else {
                    newX = e->pos.x + sz.x;       // exits to the right of the wall
                    pb.touchingLeftWall = true;   // the wall face now touches the body's left side
                    pb.contactNormal += Vec2(1.0f, 0.0f);
                }
            }
            // In the fallback branch the velocity is already zero, so this
            // assignment only zeroes a real motion into the wall.
            pb.vel.x = 0.0f;
        }
    }
    pb.pos.x = newX;

    // --- Y axis ---
    float newY = pb.pos.y + pb.vel.y * dt;
    for (auto& e : scene.getEntities()) {
        if (!e->isStaticCollider || e->isTrigger) continue;
        if (e->layer != player->layer) continue;
        if (e->broken) continue; // shattered walls no longer exist for physics
        const sf::Vector2f sz = e->shape.getSize();
        if (pb.pos.x + pb.w > e->pos.x && pb.pos.x < e->pos.x + sz.x &&
            newY + pb.h > e->pos.y && newY < e->pos.y + sz.y) {
            // A hard downward landing shatters the matching breakable walls
            // before the resolution, so the body smashes through and keeps
            // falling instead of stopping on top.
            if ((e->wallType == WallType::BreakStomp || e->wallType == WallType::BreakAny) &&
                pb.vel.y >= BREAK_STOMP_MIN_SPEED) {
                e->broken = true;
                continue;
            }
            if (pb.vel.y > 0.0f) {
                newY = e->pos.y - pb.h;
                pb.vel.y = 0.0f;
                pb.onGround = true;
                pb.touchingFloor = true;
                pb.contactNormal += Vec2(0.0f, -1.0f);
            } else if (pb.vel.y < 0.0f) {
                newY = e->pos.y + sz.y;
                pb.vel.y = 0.0f;
                pb.contactNormal += Vec2(0.0f, 1.0f);
            } else {
                // Same standstill fallback as on the X axis: a motionless
                // body overlapping solid geometry exits through the face of
                // least penetration. Exiting downward reports a floor
                // contact, so a body dropped into a cramped gap still lands.
                const float penFromTopFace    = newY + pb.h - e->pos.y;       // depth past the wall's top edge
                const float penFromBottomFace = e->pos.y + sz.y - newY;       // depth past the wall's bottom edge
                if (penFromTopFace < penFromBottomFace) {
                    newY = e->pos.y - pb.h;
                    pb.onGround = true;
                    pb.touchingFloor = true;
                    pb.contactNormal += Vec2(0.0f, -1.0f);
                } else {
                    newY = e->pos.y + sz.y;
                    pb.contactNormal += Vec2(0.0f, 1.0f);
                }
            }
        }
    }
    pb.pos.y = newY;

    // A body squeezed into an inside corner touches two opposite faces at
    // once: the accumulated raw sum would have length 2, so it is normalized
    // into a diagonal direction that points out of the corner.
    if (pb.contactNormal.lenSq() > 1.0f) pb.contactNormal = pb.contactNormal.norm();

    // Level bounds (horizontal). The camera clamp keeps the player visible.
    if (pb.pos.x < 0.0f) { pb.pos.x = 0.0f; pb.vel.x = 0.0f; }
    if (pb.pos.x > static_cast<float>(LEVEL_W) - pb.w) {
        pb.pos.x = static_cast<float>(LEVEL_W) - pb.w;
        pb.vel.x = 0.0f;
    }
}

bool Game::hasStandRoom() const {
    const PhysicsBody& pb = *player->body;
    // The grown box keeps the current feet line and extends upward to the
    // full player height.
    const float grownTop = pb.pos.y + pb.h - static_cast<float>(PLAYER_H);
    for (const auto& e : scene.getEntities()) {
        if (!e->isStaticCollider || e->isTrigger) continue;
        if (e->layer != player->layer) continue;
        if (e->broken) continue; // shattered walls leave room by definition
        const sf::Vector2f sz = e->shape.getSize();
        if (pb.pos.x + pb.w > e->pos.x && pb.pos.x < e->pos.x + sz.x &&
            grownTop + PLAYER_H > e->pos.y && grownTop < e->pos.y + sz.y)
            return false;
    }
    return true;
}

void Game::refitHitbox() {
    PhysicsBody& pb = *player->body;
    const float targetH = (moveState == MoveState::Slide)
        ? static_cast<float>(SLIDE_HEIGHT)
        : static_cast<float>(PLAYER_H);
    if (targetH == pb.h) return;
    if (targetH < pb.h) {
        // Shrinking: anchor the feet so the body drops into the low profile.
        pb.pos.y += pb.h - targetH;
        pb.h = targetH;
    } else if (hasStandRoom()) {
        // Growing: rise back up, again keeping the feet anchored.
        pb.pos.y -= targetH - pb.h;
        pb.h = targetH;
    }
    // When there is no headroom the low profile simply stays in place until
    // hasStandRoom() clears it.
}
