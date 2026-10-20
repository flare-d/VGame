#include "Renderer.h"
#include <cstdio>
#include "params/WorldParams.h"
#include "params/LevelParams.h"

Renderer::Renderer() { loadFont(); }

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

void Renderer::render(sf::RenderWindow& window, const PhysicsBody& player,
                      const std::vector<Wall>& walls,
                      const Vec2& dbgFin, const Vec2& dbgFg,
                      const Vec2& dbgFf, const Vec2& dbgFd,
                      float coyoteTimer, float jumpBufferTimer, float fps,
                      float camX, float camY) {
    window.clear(sf::Color::Black);

    // Background covers the whole level; anything outside stays black.
    sf::RectangleShape bg({static_cast<float>(LEVEL_W), static_cast<float>(LEVEL_H)});
    bg.setPosition({0.0f, 0.0f});
    bg.setFillColor(sf::Color(18, 18, 26));
    window.draw(bg);

    drawGrid(window);
    drawWalls(window, walls);
    drawPlayer(window, player);

    if (showDebug) {
        drawVectors(window, player, dbgFin, dbgFg, dbgFf, dbgFd);
        drawDebugPanel(window, player, player.acc, coyoteTimer, jumpBufferTimer, fps, camX, camY);
    }
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

void Renderer::drawWalls(sf::RenderWindow& w, const std::vector<Wall>& walls) {
    for (const auto& wall : walls) {
        sf::RectangleShape rect({wall.w, wall.h});
        rect.setPosition({wall.x, wall.y});
        rect.setFillColor(sf::Color(60, 90, 60));
        w.draw(rect);
    }
}

void Renderer::drawPlayer(sf::RenderWindow& w, const PhysicsBody& player) {
    sf::RectangleShape body({PLAYER_W, PLAYER_H});
    body.setPosition({static_cast<float>(player.px()), static_cast<float>(player.py())});
    body.setFillColor(player.onGround ? sf::Color(220, 50, 50) : sf::Color(255, 140, 40));
    w.draw(body);
}

void Renderer::drawVectors(sf::RenderWindow& w, const PhysicsBody& player,
                           const Vec2& fin, const Vec2& fg,
                           const Vec2& ff, const Vec2& fd) {
    const sf::Vector2f center(
        static_cast<float>(player.px() + PLAYER_W / 2),
        static_cast<float>(player.py() + PLAYER_H / 2));
    // Force-to-pixel scale: gravity (6000 N) draws as a 60 px arrow on the 320 px screen.
    constexpr float forceScale = 0.01f;
    drawArrow(w, center, fin * forceScale, sf::Color(255, 40, 40));
    drawArrow(w, center, fg  * forceScale, sf::Color(60, 160, 255));
    drawArrow(w, center, ff  * forceScale, sf::Color(255, 230, 60));
    drawArrow(w, center, fd  * forceScale, sf::Color(200, 60, 200));
    // Velocity uses its own scale; drawArrow clamps the drawn length either way.
    drawArrow(w, center, player.vel * 1.5f, sf::Color(60, 255, 80));
}

void Renderer::drawArrow(sf::RenderWindow& w, sf::Vector2f from, Vec2 v, sf::Color c) {
    if (v.lenSq() < 0.5f) return;
    // Clamps the drawn length so strong vectors stay readable on the small screen.
    constexpr float maxLen = 60.0f;
    const float l = v.len();
    if (l > maxLen) v = v * (maxLen / l);
    sf::Vertex line[2] = {
        {from, c},
        {{from.x + v.x, from.y + v.y}, c}
    };
    w.draw(line, 2, sf::PrimitiveType::Lines);
}

void Renderer::drawDebugPanel(sf::RenderWindow& w, const PhysicsBody& player,
                              const Vec2& acc, float coyote, float jumpBuf, float fps,
                              float camX, float camY) {
    // The panel is anchored to the view corner so it stays on screen while the
    // camera moves through the world.
    const float left = camX - SCREEN_W / 2.0f + 2.0f;
    const float top  = camY - SCREEN_H / 2.0f + 2.0f;

    sf::RectangleShape panel({130, 106});
    panel.setPosition({left, top});
    panel.setFillColor(sf::Color(0, 0, 0, 200));
    w.draw(panel);

    if (!fontLoaded) return;

    auto line = [&](const std::string& s, int x, int y) {
        sf::Text txt(font);
        txt.setString(s);
        txt.setCharacterSize(8);
        txt.setFillColor(sf::Color(220, 220, 220));
        txt.setPosition({static_cast<float>(x), static_cast<float>(y)});
        w.draw(txt);
    };

    int x = static_cast<int>(left) + 3, y = static_cast<int>(top) + 2;
    const int step = 12;
    line("POS  " + fmt(player.pos.x) + "," + fmt(player.pos.y), x, y); y += step;
    line("VEL  " + fmt(player.vel.x) + "," + fmt(player.vel.y), x, y); y += step;
    line("ACC  " + fmt(acc.x) + "," + fmt(acc.y), x, y); y += step;
    line("GND  " + std::string(player.onGround ? "YES" : "NO "), x, y); y += step;
    line("COY  " + fmt(coyote), x, y); y += step;
    line("BUF  " + fmt(jumpBuf), x, y); y += step;
    line("FPS  " + fmt(fps), x, y); y += step;
    line("CAM  " + fmt(camX) + "," + fmt(camY), x, y);
}
