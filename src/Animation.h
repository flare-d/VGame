#pragma once
#include <SFML/Graphics.hpp>
#include <string>

// One animation clip: a horizontal row of equal-sized frames inside one
// texture sheet. Frames are indexed left to right; the animator turns the
// elapsed time into a frame index, the renderer maps that index to a
// texture rectangle.
struct AnimationClip {
    std::string texture;      // path relative to the assets directory
    int frameWidth = 0;       // px, one frame
    int frameHeight = 0;      // px, one frame
    int frames = 1;           // total frames in the row
    float secondsPerFrame = 0.1f;
    bool loop = true;         // loop forever, or clamp on the last frame
};

// Advances a clip with gameplay time and remembers which clip is active.
// The clip is stored by value, so switching states never leaves dangling
// pointers; playing the same clip again continues seamlessly instead of
// restarting.
class Animator {
    AnimationClip clip;
    bool hasClip = false;
    float time = 0.0f;
    int current = 0;

public:
    // Activates a clip. A different clip restarts at frame zero; the same
    // clip keeps its current frame and time.
    void play(const AnimationClip& c) {
        if (!hasClip || c.texture != clip.texture || c.frames != clip.frames ||
            c.frameWidth != clip.frameWidth) {
            clip = c;
            time = 0.0f;
            current = 0;
        }
        hasClip = true;
    }

    // Advances the animation. Looping clips wrap around; one-shot clips
    // clamp on the last frame until play() switches to something else.
    void update(float dt) {
        if (!hasClip || clip.frames <= 1 || clip.secondsPerFrame <= 0.0f) return;
        time += dt;
        const float total = clip.secondsPerFrame * static_cast<float>(clip.frames);
        if (clip.loop) {
            if (time >= total) time -= total * static_cast<int>(time / total);
            current = static_cast<int>(time / clip.secondsPerFrame);
            if (current >= clip.frames) current = clip.frames - 1;
        } else if (time >= total) {
            current = clip.frames - 1;
        } else {
            current = static_cast<int>(time / clip.secondsPerFrame);
        }
    }

    // True while a clip is active. Rendering engages sprite drawing only
    // for animated entities whose texture actually loads.
    bool active() const { return hasClip; }
    const AnimationClip& activeClip() const { return clip; }
    int frameIndex() const { return current; }

    // The texture rectangle of the current frame inside the sheet.
    sf::IntRect textureRect() const {
        return sf::IntRect({current * clip.frameWidth, 0},
                           {clip.frameWidth, clip.frameHeight});
    }
};
