#pragma once

// Downward contact speed that shatters a BreakStomp block. Roughly one
// tile of free fall already exceeds it, while walking, jumping through
// and brushing past never do: the block demands an actual landing.
constexpr float BREAK_STOMP_MIN_SPEED = 220.0f; // px/s downward at contact

// Sideways contact speed that shatters a BreakImpact block. It sits above
// running speed (~130 px/s), the wall-jump kick (~52 px/s) and the slide
// exit speed, so the only reliable source of this force is the dash
// (260 px/s). Impact blocks are, in effect, dash gates.
constexpr float BREAK_IMPACT_MIN_SPEED = 240.0f; // px/s sideways at contact
