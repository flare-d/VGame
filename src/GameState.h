#pragma once

// Top-level game states. Only Playing runs the simulation right now; the
// remaining states freeze the update so rendering and input polling continue
// while gameplay is suspended. They will gain dedicated behavior later.
enum class GameState {
    Playing,
    Paused,
    Dialog,
    Dead
};
