#pragma once

// A jump stays possible this long after the player leaves the ground.
constexpr float COYOTE_TIME = 0.10f; // s

// A jump press is remembered this long and fires as soon as it becomes valid.
constexpr float JUMP_BUFFER_TIME = 0.10f; // s
