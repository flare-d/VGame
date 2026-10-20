#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Input.h"
#include "KeyBindings.h"
#include "Settings.h"

class Window {
    sf::RenderWindow window;
    sf::View gameView;
    Input input;
    KeyBindings bindings;
public:
    // exePath anchors the key bindings lookup; the Settings decide the video
    // mode (windowed size or fullscreen) and vsync. An empty exePath falls
    // back to the current working directory for file lookup.
    Window(const std::string& exePath, const Settings& settings);
    bool isOpen() const;
    void close();
    void pollEvents();
    void resetOneShotInputs();
    void resize(unsigned int w, unsigned int h);
    void display();
    const Input& getInput() const;
    sf::RenderWindow& getWindow();

    // Centers the game view on a world position. Called once per frame with
    // the camera position; the letterbox viewport is left untouched.
    void setCameraCenter(float x, float y);

    // Grants read access to the current key layout.
    const KeyBindings& getBindings() const { return bindings; }

    // Rebinds an action to a different key. Takes effect on the next event poll.
    void rebind(Action action, sf::Keyboard::Key key) { bindings.rebind(action, key); }
};
