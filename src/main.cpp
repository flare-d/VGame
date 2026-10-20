#include "Window.h"
#include "Game.h"
#include "Renderer.h"
#include "Settings.h"
#include <SFML/System/Clock.hpp>
#include <string>

// The simulation always advances in equal fixed increments, so physics behavior
// is identical at any frame rate. Rendering stays decoupled and runs every frame.
namespace {
constexpr float FIXED_DT  = 1.0f / 120.0f; // physics step: 120 Hz
constexpr int   MAX_STEPS = 8;             // physics steps per frame before dropping the backlog

// Extracts the directory part of the executable path so external files
// (key bindings, settings, save game, assets) resolve next to the binary no
// matter which working directory launches the program.
std::string dirOf(const std::string& exePath) {
    const size_t slash = exePath.find_last_of("/\\");
    if (slash == std::string::npos) return "";
    return exePath.substr(0, slash + 1);
}
} // namespace

int main(int argc, char* argv[]) {
    // The executable path anchors every external file lookup; when the host
    // provides no argv at all the fallbacks use the working directory.
    const std::string exePath = (argc > 0 && argv[0]) ? argv[0] : "";
    const std::string exeDir = dirOf(exePath);

    // Player preferences load before the window exists: the video mode,
    // vsync and pacing come from the settings file.
    Settings settings;
    readSettings(exeDir + "settings.json", settings);

    Window window(exePath, settings);
    Game game(exeDir + "savegame.json");
    Renderer renderer(exeDir + "assets/");
    sf::Clock clock;
    float accumulator = 0.0f;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.25f) dt = 0.25f; // clamps long stalls (debugger breakpoints, window dragging)

        window.pollEvents();

        // Escape toggles the pause state instead of quitting: the simulation
        // freezes through Game::update while Paused, and the renderer draws
        // the pause veil. Closing the window is the only way to exit.
        if (window.getInput().quit) {
            if (game.getState() == GameState::Playing) game.setState(GameState::Paused);
            else if (game.getState() == GameState::Paused) game.setState(GameState::Playing);
        }

        game.setInput(
            window.getInput().left,
            window.getInput().right,
            window.getInput().up,
            window.getInput().down,
            window.getInput().jumpPressed,
            window.getInput().jumpHeld,
            window.getInput().grabHeld,
            window.getInput().run,
            window.getInput().dashPressed);

        // Test hooks that act directly on the game world and persist state.
        if (window.getInput().toggleDebug) renderer.toggleDebug();
        if (window.getInput().debugHurt) game.applyDamage();
        if (window.getInput().savePressed) game.saveGame();
        if (window.getInput().loadPressed) game.loadGame();

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
            game.getScene(),
            game.getPlayer(),
            game.getDbgFin(),
            game.getDbgFg(),
            game.getDbgFf(),
            game.getDbgFd(),
            game.getCoyoteTimer(),
            game.getJumpBufferTimer(),
            game.getStunTimer(),
            game.getOxygen(),
            game.getStamina(),
            game.getDashAvailable(),
            1.0f / dt,
            game.getCamera().x(),
            game.getCamera().y(),
            game.getMoveStateName(),
            game.getState() == GameState::Paused);

        window.display();
    }
    return 0;
}
