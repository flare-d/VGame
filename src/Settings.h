#pragma once
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "SaveFile.h" // reuse the tolerant number/array extraction helpers

// Player-facing preferences, stored next to the executable as settings.json
// and edited by hand for now; the settings menu of the UI stage reads and
// writes the same struct. Gameplay constants never live here: they stay in
// the params/ headers so tuning cannot leak into player preferences.
struct Settings {
    bool fullscreen = false;
    bool vsync = false;
    int windowWidth = 1280;
    int windowHeight = 720;
    float masterVolume = 100.0f; // percent, 0..100
    float musicVolume = 100.0f;  // percent, applied on top of the master value
    float soundVolume = 100.0f;  // percent, applied on top of the master value
};

// Layout version of the settings file; incompatible files are ignored and
// the defaults stay active.
constexpr int SETTINGS_VERSION = 1;

// Serializes the settings as a small versioned JSON document.
inline bool writeSettings(const std::string& path, const Settings& s) {
    std::ofstream out(path);
    if (!out) return false;
    char buf[384];
    std::snprintf(buf, sizeof(buf),
        "{\n"
        "  \"version\": %d,\n"
        "  \"fullscreen\": %d,\n"
        "  \"vsync\": %d,\n"
        "  \"windowWidth\": %d,\n"
        "  \"windowHeight\": %d,\n"
        "  \"masterVolume\": %.0f,\n"
        "  \"musicVolume\": %.0f,\n"
        "  \"soundVolume\": %.0f\n"
        "}\n",
        SETTINGS_VERSION,
        s.fullscreen ? 1 : 0, s.vsync ? 1 : 0,
        s.windowWidth, s.windowHeight,
        s.masterVolume, s.musicVolume, s.soundVolume);
    out << buf;
    return static_cast<bool>(out);
}

// Reads the settings back; missing keys keep the struct defaults, a missing
// file or a version mismatch reports failure so the caller keeps defaults.
inline bool readSettings(const std::string& path, Settings& s) {
    std::ifstream in(path);
    if (!in) return false;
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();

    float version = 0.0f;
    if (!save_detail::extractNumber(text, "version", version)) return false;
    if (static_cast<int>(version) != SETTINGS_VERSION) return false;

    float v = 0.0f;
    if (save_detail::extractNumber(text, "fullscreen", v)) s.fullscreen = v != 0.0f;
    if (save_detail::extractNumber(text, "vsync", v)) s.vsync = v != 0.0f;
    float w = static_cast<float>(s.windowWidth);
    if (save_detail::extractNumber(text, "windowWidth", w) && w >= 320.0f)
        s.windowWidth = static_cast<int>(w);
    float h = static_cast<float>(s.windowHeight);
    if (save_detail::extractNumber(text, "windowHeight", h) && h >= 180.0f)
        s.windowHeight = static_cast<int>(h);
    float vol = 0.0f;
    if (save_detail::extractNumber(text, "masterVolume", vol))
        s.masterVolume = vol < 0.0f ? 0.0f : (vol > 100.0f ? 100.0f : vol);
    if (save_detail::extractNumber(text, "musicVolume", vol))
        s.musicVolume = vol < 0.0f ? 0.0f : (vol > 100.0f ? 100.0f : vol);
    if (save_detail::extractNumber(text, "soundVolume", vol))
        s.soundVolume = vol < 0.0f ? 0.0f : (vol > 100.0f ? 100.0f : vol);
    return true;
}
