#pragma once

// High-level game actions, independent of the physical keys bound to them.
// The Count entry must stay last: it marks the number of actions.
enum class Action {
    Left,
    Right,
    Up,
    Down,
    Jump,
    Grab,
    Run,
    Dash,
    ToggleDebug,
    DebugHurt,
    Save,
    Load,
    Quit,
    Count
};

// Instantaneous input state consumed by the game logic every frame.
// One-shot flags are cleared after each frame by Window::resetOneShotInputs.
struct Input {
    // Held-state actions.
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool run = false;

    // Actions with distinct press and hold semantics. The pressed flag is true
    // only on the frame the key went down and feeds the input buffer.
    bool jumpPressed = false;
    bool jumpHeld = false;
    bool grabHeld = false;
    bool dashPressed = false;
    bool dashHeld = false;

    // One-shot flags.
    bool toggleDebug = false;
    bool debugHurt = false; // test hook: fires a damage event on the player
    bool savePressed = false; // test hook: writes the world snapshot to disk
    bool loadPressed = false; // test hook: reloads the world snapshot from disk
    bool quit = false;
};
