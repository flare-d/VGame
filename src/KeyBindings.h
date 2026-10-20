#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <string>
#include "Input.h"

// Maps high-level game actions to physical keyboard keys.
// Bindings can be changed at runtime through rebind() and can be loaded from /
// saved to a small text configuration file (one "action = key" entry per line).
class KeyBindings {
public:
    // Assigns the default layout: arrows to move, Z to jump, V to grab,
    // LShift to run, F3 for debug, Escape to quit.
    KeyBindings();

    // Returns the key currently assigned to the requested action.
    sf::Keyboard::Key get(Action action) const;

    // Assigns a new key to an action. Takes effect immediately.
    void rebind(Action action, sf::Keyboard::Key key);

    // Loads bindings from a text file. A missing file, unknown actions and
    // unknown key names leave the corresponding bindings untouched.
    // Returns true when the file was opened successfully.
    bool loadFromFile(const std::string& path);

    // Writes the current bindings to a text file, including a short syntax guide.
    // Returns true on success.
    bool saveToFile(const std::string& path) const;

private:
    sf::Keyboard::Key keys[static_cast<int>(Action::Count)];
};
