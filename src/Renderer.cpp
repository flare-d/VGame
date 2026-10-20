#include "Renderer.h"
#include <cstdio>
#include "params/WorldParams.h"
#include "params/LevelParams.h"
#include "params/RenderParams.h"

Renderer::Renderer(const std::string& assetsDirectory) : assetsDir(assetsDirectory) {
    loadFont();
}

void Renderer::loadFont() {
    std::vector<std::string> paths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf"
    };
    for (const auto& p : paths) {
        if (font.openFromFile(p)) { fontLoaded = true; break; }
    }
}

void Renderer::render(sf::RenderWindow& window, const Scene& scene, const PhysicsBody& player,
                      const Vec2& fin, const Vec2& fg, const Vec2& ff, const Vec2& fd,
                      float coyoteTimer, float jumpBufferTimer, float stunTimer,
                      float oxygen, float stamina, bool dashAvailable, float fps,
                      float camX, float camY, const char* moveState, bool paused) {
    window.clear(sf::Color::Black);

    // Background covers the whole level; anything outside stays black.
    sf::RectangleShape bg({static_cast<float>(LEVEL_W), static_cast<float>(LEVEL_H)});
    bg.setPosition({0.0f, 0.0f});
    bg.setFillColor(sf::Color(18, 18, 26));
    window.draw(bg);

    drawGrid(window);
    drawWater(window);
    drawEntities(window, scene, player, stunTimer);

    if (showDebug) {
        drawVectors(window, player, fin, fg, ff, fd);
        drawDebugPanel(window, player, player.acc, coyoteTimer, jumpBufferTimer,
                       stunTimer, oxygen, stamina, dashAvailable, fps,
                       camX, camY, moveState);
    }

    // The pause veil draws last so it covers the frame, the debug panel and
    // every arrow alike.
    if (paused) drawPauseOverlay(window, camX, camY);
}

void Renderer::drawGrid(sf::RenderWindow& w) {
    sf::Color c(35, 35, 45);
    for (int x = 0; x <= LEVEL_W; x += 16) {
        sf::Vertex v[2] = {
            {{static_cast<float>(x), 0.0f}, c},
            {{static_cast<float>(x), static_cast<float>(LEVEL_H)}, c}
        };
        w.draw(v, 2, sf::PrimitiveType::Lines);
    }
    for (int y = 0; y <= LEVEL_H; y += 16) {
        sf::Vertex v[2] = {
            {{0.0f, static_cast<float>(y)}, c},
            {{static_cast<float>(LEVEL_W), static_cast<float>(y)}, c}
        };
        w.draw(v, 2, sf::PrimitiveType::Lines);
    }
}

void Renderer::drawWater(sf::RenderWindow& w) {
    // Water volumes are drawn as translucent fills with a bright surface line
    // so they read as a medium, not as solid geometry.
    for (unsigned i = 0; i < WATER_ZONE_COUNT; ++i) {
        const WaterZoneData& z = WATER_ZONES[i];
        sf::RectangleShape body({z.w, z.h});
        body.setPosition({z.x, z.y});
        body.setFillColor(sf::Color(40, 90, 200, 110));
        w.draw(body);

        const sf::Color surface(140, 190, 255);
        sf::Vertex line[2] = {
            {{z.x, z.y}, surface},
            {{z.x + z.w, z.y}, surface}
        };
        w.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void Renderer::drawEntities(sf::RenderWindow& w, const Scene& scene, const PhysicsBody& player,
                            float stunTimer) {
    // The stun blink turns the player white on a fixed cadence for the whole
    // recovery window, so immobilization reads even without the debug panel.
    const bool blinkOn = stunTimer > 0.0f &&
        (static_cast<int>(stunTimer * 8.0f) % 2 == 0);

    for (const auto& e : scene.getEntities()) {
        if (e->broken) continue; // shattered walls leave no rubble placeholder

        // Animated entities draw their current sprite-sheet frame when the
        // texture loads; a missing sheet falls through to the placeholder
        // rectangle, so the animation pipeline works with an empty assets
        // folder and picks the art up as soon as the files appear.
        if (e->animator.active()) {
            const AnimationClip& clip = e->animator.activeClip();
            if (clip.frameWidth > 0 && clip.frames > 0) {
                const sf::Texture& tex = cache.texture(assetsDir + clip.texture);
                if (tex.getSize().x > 0) {
                    sf::Sprite sprite(tex);
                    sprite.setTextureRect(e->animator.textureRect());
                    if (e->body) {
                        // Dynamic bodies follow the physics box exactly.
                        sprite.setPosition({e->body->pos.x, e->body->pos.y});
                    } else {
                        sprite.setPosition({e->pos.x, e->pos.y});
                    }
                    if (e->body && blinkOn) sprite.setColor(sf::Color(255, 255, 255));
                    w.draw(sprite);
                    continue;
                }
            }
        }

        sf::RectangleShape sprite = e->shape; // copy so per-frame tinting never mutates the entity
        if (e->body) {
            // Dynamic bodies follow the physics box exactly, including the
            // low slide profile.
            sprite.setSize({e->body->w, e->body->h});
            sprite.setPosition({e->body->pos.x, e->body->pos.y});
        } else {
            sprite.setPosition({e->pos.x, e->pos.y});
        }

        if (e->body) {
            // Dynamic bodies (the player) are tinted by ground state and
            // blink white while the damage stun is active.
            if (blinkOn) sprite.setFillColor(sf::Color(255, 255, 255));
            else sprite.setFillColor(player.onGround ? sf::Color(220, 50, 50)
                                                     : sf::Color(255, 140, 40));
        } else if (e->wallType == WallType::BreakStomp) {
            // Breakable blocks are tinted by the force they demand: warm
            // yellow for a downward smash.
            sprite.setFillColor(sf::Color(150, 120, 40));
        } else if (e->wallType == WallType::BreakImpact) {
            // Cold blue for a sideways impact, so the two gate kinds are
            // distinguishable before the player has the dash.
            sprite.setFillColor(sf::Color(60, 90, 150));
        } else if (e->wallType == WallType::BreakAny) {
            sprite.setFillColor(sf::Color(140, 70, 130));
        } else if (e->isTrigger) {
            // Triggers are drawn as outlines so they read as volumes, not solids.
            sprite.setFillColor(sf::Color::Transparent);
            sprite.setOutlineColor(sf::Color(120, 200, 255));
            sprite.setOutlineThickness(1.0f);
        }
        w.draw(sprite);
    }
}

void Renderer::drawVectors(sf::RenderWindow& w, const PhysicsBody& player,
                           const Vec2& fin, const Vec2& fg,
                           const Vec2& ff, const Vec2& fd) {
    const sf::Vector2f center(
        static_cast<float>(player.px() + static_cast<int>(player.w) / 2),
        static_cast<float>(player.py() + static_cast<int>(player.h) / 2));
    drawArrow(w, center, fin * DEBUG_FORCE_ARROW_SCALE, sf::Color(255, 40, 40));
    drawArrow(w, center, fg  * DEBUG_FORCE_ARROW_SCALE, sf::Color(60, 160, 255));
    drawArrow(w, center, ff  * DEBUG_FORCE_ARROW_SCALE, sf::Color(255, 230, 60));
    drawArrow(w, center, fd  * DEBUG_FORCE_ARROW_SCALE, sf::Color(200, 60, 200));
    // Velocity uses its own scale because velocity magnitudes differ wildly
    // from force magnitudes; drawArrow clamps the drawn length either way.
    drawArrow(w, center, player.vel * DEBUG_VELOCITY_ARROW_SCALE, sf::Color(60, 255, 80));
}

void Renderer::drawArrow(sf::RenderWindow& w, sf::Vector2f from, Vec2 v, sf::Color c) {
    if (v.lenSq() < 0.5f) return;
    // Clamps the drawn length so strong vectors stay readable on the small screen.
    const float l = v.len();
    if (l > DEBUG_ARROW_MAX_LEN) v = v * (DEBUG_ARROW_MAX_LEN / l);
    sf::Vertex line[2] = {
        {from, c},
        {{from.x + v.x, from.y + v.y}, c}
    };
    w.draw(line, 2, sf::PrimitiveType::Lines);
}

void Renderer::drawDebugPanel(sf::RenderWindow& w, const PhysicsBody& player,
                              const Vec2& acc, float coyote, float jumpBuf,
                              float stunTimer, float oxygen, float stamina,
                              bool dashAvailable, float fps,
                              float camX, float camY, const char* moveState) {
    // The panel is anchored to the view corner so it stays on screen while the
    // camera moves through the world.
    const float left = camX - SCREEN_W / 2.0f + 2.0f;
    const float top  = camY - SCREEN_H / 2.0f + 2.0f;

    sf::RectangleShape panel({DEBUG_PANEL_W, DEBUG_PANEL_H});
    panel.setPosition({left, top});
    panel.setFillColor(sf::Color(0, 0, 0, 200));
    w.draw(panel);

    if (!fontLoaded) return;

    auto line = [&](const std::string& s, int x, int y) {
        sf::Text txt(font);
        txt.setString(s);
        txt.setCharacterSize(DEBUG_FONT_SIZE);
        txt.setFillColor(sf::Color(220, 220, 220));
        txt.setPosition({static_cast<float>(x), static_cast<float>(y)});
        w.draw(txt);
    };

    // Compact contact report: F = floor, L/R = side walls, '-' = no contact.
    std::string contact;
    contact += player.touchingFloor ? 'F' : '-';
    contact += player.touchingLeftWall ? 'L' : '-';
    contact += player.touchingRightWall ? 'R' : '-';

    int x = static_cast<int>(left) + 3, y = static_cast<int>(top) + 2;
    const int step = DEBUG_LINE_STEP;
    line("POS  " + fmt(player.pos.x) + "," + fmt(player.pos.y), x, y); y += step;
    line("VEL  " + fmt(player.vel.x) + "," + fmt(player.vel.y), x, y); y += step;
    line("ACC  " + fmt(acc.x) + "," + fmt(acc.y), x, y); y += step;
    line("ST   " + std::string(moveState), x, y); y += step;
    line("GND  " + std::string(player.onGround ? "YES" : "NO "), x, y); y += step;
    line("COY  " + fmt(coyote), x, y); y += step;
    line("BUF  " + fmt(jumpBuf), x, y); y += step;
    line("STUN " + fmt(stunTimer), x, y); y += step;
    line("OXY  " + fmt(oxygen), x, y); y += step;
    line("STA  " + fmt(stamina), x, y); y += step;
    line("DASH " + std::string(dashAvailable ? "RDY" : "---"), x, y); y += step;
    line("CNT  " + contact, x, y); y += step;
    line("FPS  " + fmt(fps), x, y); y += step;
    line("CAM  " + fmt(camX) + "," + fmt(camY), x, y);
}

void Renderer::drawPauseOverlay(sf::RenderWindow& w, float camX, float camY) {
    // The veil covers exactly the game view rectangle: it is positioned in
    // world coordinates so it stays glued to the screen while the frozen
    // camera sits still.
    sf::RectangleShape veil({static_cast<float>(SCREEN_W), static_cast<float>(SCREEN_H)});
    veil.setPosition({camX - SCREEN_W / 2.0f, camY - SCREEN_H / 2.0f});
    veil.setFillColor(sf::Color(0, 0, 0, 160));
    w.draw(veil);

    if (!fontLoaded) return;

    sf::Text label(font);
    label.setString("PAUSED");
    label.setCharacterSize(PAUSE_FONT_SIZE);
    label.setFillColor(sf::Color(230, 230, 230));
    // The origin is moved to the text bounding box center so setPosition can
    // place the label exactly on the view center without size estimates.
    const auto bounds = label.getLocalBounds();
    label.setOrigin({bounds.position.x + bounds.size.x / 2.0f,
                     bounds.position.y + bounds.size.y / 2.0f});
    label.setPosition({camX, camY});
    w.draw(label);
}
