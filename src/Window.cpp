#include "Window.h"
#include "params/WorldParams.h"

namespace {

// Extracts the directory part of the executable path (everything up to and
// including the last path separator) so the key bindings file can be opened
// next to the executable. A path without a separator yields an empty string,
// which makes the later file open fall back to the current working directory.
std::string dirOf(const std::string& exePath) {
    const size_t slash = exePath.find_last_of("/\\");
    if (slash == std::string::npos) return "";
    return exePath.substr(0, slash + 1);
}

} // namespace

Window::Window(const std::string& exePath, const Settings& settings)
    : window(
          sf::VideoMode({static_cast<unsigned>(settings.windowWidth),
                         static_cast<unsigned>(settings.windowHeight)}),
          "Physics | 320x180 | Arrows=move Z=jump X=dash V=grab Shift=run F3=debug F4=hurt F5=save F9=load ESC=pause",
          settings.fullscreen ? sf::Style::Fullscreen : sf::Style::Default)
{
    // Optional key bindings file, resolved relative to the executable. If
    // keybinds.cfg exists there its entries override the default layout for
    // the current session; a missing file keeps the built-in defaults.
    bindings.loadFromFile(dirOf(exePath) + "keybinds.cfg");

    gameView = sf::View(sf::FloatRect({0.0f, 0.0f},
        {static_cast<float>(SCREEN_W), static_cast<float>(SCREEN_H)}));
    window.setView(gameView);

    // Vsync and the frame rate limiter are mutually exclusive pacing
    // mechanisms; with vsync on the display dictates the pace.
    window.setVerticalSyncEnabled(settings.vsync);
    if (!settings.vsync) window.setFramerateLimit(60);

    // Computes the initial letterbox viewport for the actual window size.
    const auto size = window.getSize();
    resize(size.x, size.y);
}

bool Window::isOpen() const { return window.isOpen(); }
void Window::close() { window.close(); }
void Window::display() { window.display(); }
const Input& Window::getInput() const { return input; }
sf::RenderWindow& Window::getWindow() { return window; }

void Window::setCameraCenter(float x, float y) {
    // setCenter keeps the view size and viewport intact.
    gameView.setCenter({x, y});
    window.setView(gameView);
}

void Window::pollEvents() {
    while (const std::optional<sf::Event> ev = window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) window.close();

        // After focus loss the OS may never deliver the matching key release
        // events, so every held key is cleared to prevent stuck inputs.
        if (ev->is<sf::Event::FocusLost>()) input = Input{};

        if (const auto* rs = ev->getIf<sf::Event::Resized>())
            resize(rs->size.x, rs->size.y);

        if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
            const sf::Keyboard::Key code = kp->code;
            if (code == bindings.get(Action::Left)) {
                input.left = true;
            } else if (code == bindings.get(Action::Right)) {
                input.right = true;
            } else if (code == bindings.get(Action::Up)) {
                input.up = true;
            } else if (code == bindings.get(Action::Down)) {
                input.down = true;
            } else if (code == bindings.get(Action::Run)) {
                input.run = true;
            } else if (code == bindings.get(Action::Jump)) {
                // Key autorepeat produces repeated KeyPressed events while the
                // key stays down. Only the first one (when the key was not held)
                // counts as a fresh press for the input buffer.
                if (!input.jumpHeld) input.jumpPressed = true;
                input.jumpHeld = true;
            } else if (code == bindings.get(Action::Grab)) {
                // Wall grip is a held action: the climb state lives exactly
                // as long as the key is down, so no edge-pressed flag exists.
                input.grabHeld = true;
            } else if (code == bindings.get(Action::Dash)) {
                // The dash is edge-triggered; the held state exists only to
                // swallow OS key autorepeat while the key stays down.
                if (!input.dashHeld) input.dashPressed = true;
                input.dashHeld = true;
            } else if (code == bindings.get(Action::ToggleDebug)) {
                input.toggleDebug = true;
            } else if (code == bindings.get(Action::DebugHurt)) {
                // Test hook for the damage system: fires a damage event
                // without any hazard in the world.
                input.debugHurt = true;
            } else if (code == bindings.get(Action::Save)) {
                // Test hook for the save system until save rooms exist.
                input.savePressed = true;
            } else if (code == bindings.get(Action::Load)) {
                // Test hook for the save system until save rooms exist.
                input.loadPressed = true;
            } else if (code == bindings.get(Action::Quit)) {
                // The Quit action is consumed by the main loop as a pause
                // toggle; closing the window still ends the program.
                input.quit = true;
            }
        }
        if (const auto* kr = ev->getIf<sf::Event::KeyReleased>()) {
            const sf::Keyboard::Key code = kr->code;
            if (code == bindings.get(Action::Left)) input.left = false;
            else if (code == bindings.get(Action::Right)) input.right = false;
            else if (code == bindings.get(Action::Up)) input.up = false;
            else if (code == bindings.get(Action::Down)) input.down = false;
            else if (code == bindings.get(Action::Run)) input.run = false;
            else if (code == bindings.get(Action::Jump)) input.jumpHeld = false;
            else if (code == bindings.get(Action::Grab)) input.grabHeld = false;
            else if (code == bindings.get(Action::Dash)) input.dashHeld = false;
        }
    }
}

void Window::resetOneShotInputs() {
    input.jumpPressed = false;
    input.dashPressed = false;
    input.toggleDebug = false;
    input.debugHurt = false;
    input.savePressed = false;
    input.loadPressed = false;
    input.quit = false;
}

void Window::resize(unsigned int w, unsigned int h) {
    // Computes a centered viewport that preserves the 320x180 aspect ratio.
    // Only the viewport changes; the view center set by the camera is kept.
    const float wa = static_cast<float>(w) / static_cast<float>(h);
    const float va = static_cast<float>(SCREEN_W) / static_cast<float>(SCREEN_H);
    float vpW = 1.0f, vpH = 1.0f, vpX = 0.0f, vpY = 0.0f;
    if (wa > va) {
        vpW = va / wa;
        vpX = (1.0f - vpW) / 2.0f;
    } else if (wa < va) {
        vpH = wa / va;
        vpY = (1.0f - vpH) / 2.0f;
    }
    gameView.setViewport(sf::FloatRect({vpX, vpY}, {vpW, vpH}));
    window.setView(gameView);
}
