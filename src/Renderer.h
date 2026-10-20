#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "PhysicsBody.h"
#include "Scene.h"
#include "Resources.h"

class Renderer {
    sf::Font font;
    bool fontLoaded = false;
    bool showDebug = false;
    std::string assetsDir;  // textures resolve relative to this directory
    ResourceCache cache;    // every texture loads at most once per run
public:
    // assetsDirectory anchors the animation texture paths ("player/run.png"
    // and the like) next to the executable.
    explicit Renderer(const std::string& assetsDirectory = "assets/");
    void toggleDebug() { showDebug = !showDebug; }
    // The four force components (input, gravity, surface, drag) feed the debug
    // arrows; camX/camY anchor the debug HUD to the view corner; moveState
    // names the current locomotion state; oxygen, stamina, stun and the dash
    // charge feed the resource lines of the debug panel. While paused is
    // true a dim veil and the PAUSED label cover the view, on top of the
    // frozen frame.
    void render(sf::RenderWindow& window, const Scene& scene, const PhysicsBody& player,
                const Vec2& fin, const Vec2& fg, const Vec2& ff, const Vec2& fd,
                float coyoteTimer, float jumpBufferTimer, float stunTimer,
                float oxygen, float stamina, bool dashAvailable, float fps,
                float camX, float camY, const char* moveState, bool paused);
private:
    void drawGrid(sf::RenderWindow& w);
    void drawWater(sf::RenderWindow& w);
    void drawEntities(sf::RenderWindow& w, const Scene& scene, const PhysicsBody& player,
                      float stunTimer);
    void drawVectors(sf::RenderWindow& w, const PhysicsBody& player,
                     const Vec2& fin, const Vec2& fg,
                     const Vec2& ff, const Vec2& fd);
    void drawDebugPanel(sf::RenderWindow& w, const PhysicsBody& player,
                        const Vec2& acc, float coyote, float jumpBuf,
                        float stunTimer, float oxygen, float stamina,
                        bool dashAvailable, float fps,
                        float camX, float camY, const char* moveState);
    void drawPauseOverlay(sf::RenderWindow& w, float camX, float camY);
    void drawArrow(sf::RenderWindow& w, sf::Vector2f from, Vec2 v, sf::Color c);
    void loadFont();
};
