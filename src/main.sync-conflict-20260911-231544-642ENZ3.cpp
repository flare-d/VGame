#include "Window.h"
#include "Game.h"
#include "Renderer.h"
#include <SFML/System/Clock.hpp>

// The simulation always advances in equal fixed increments, so physics behavior
// is identical at any frame rate. Rendering stays decoupled and runs every frame.
namespace {
constexpr float FIXED_DT  = 1.0f / 120.0f; // physics step: 120 Hz
constexpr int   MAX_STEPS = 8;             // physics steps per frame before dropping the backlog
}

int main() {
    Window window;
    Game game;
    Renderer renderer;
    sf::Clock clock;
    float accumulator = 0.0f;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.25f) dt = 0.25f; // clamps long stalls (debugger breakpoints, window dragging)

        window.pollEvents();
        if (window.getInput().quit) window.close();

        game.setInput(
            window.getInput().left,
            window.getInput().right,
            window.getInput().jumpPressed,
            window.getInput().jumpHeld);

        if (window.getInput().toggleDebug) renderer.toggleDebug();
        window.resetOneShotInputs();

        accumulator += dt;
        int steps = 0;
        while (accumulator >= FIXED_DT && steps < MAX_STEPS) {
            game.update(FIXED_DT);
            accumulator -= FIXED_DT;
            ++steps;
        }
        // A machine too slow for 8 steps per frame drops the backlog instead of
        // entering a death spiral of ever-growing catch-up work.
        if (steps == MAX_STEPS) accumulator = 0.0f;

        // Moves the view to the camera position before drawing.
        window.setCameraCenter(game.getCamera().x(), game.getCamera().y());

        renderer.render(
            window.getWindow(),
            game.getPlayer(),
            game.getWalls(),
            game.getDbgFin(),
            game.getDbgFg(),
            game.getDbgFf(),
            game.getDbgFd(),
            game.getCoyoteTimer(),
            game.getJumpBufferTimer(),
            1.0f / dt,
            game.getCamera().x(),
            game.getCamera().y());

        window.display();
    }
    return 0;
}
