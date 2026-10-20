#pragma once
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Plain-data snapshot of everything a death rollback must restore: the
// player state and the list of shattered breakable walls. Nothing here
// knows about entities or physics; Game converts its live state into this
// struct and back, which keeps the file format independent of the
// simulation internals.
struct GameSnapshot {
    float playerX = 0.0f;
    float playerY = 0.0f;
    int facing = 1;             // last horizontal direction, +1 right / -1 left
    bool dashAvailable = true;  // the single air dash charge
    float oxygen = 100.0f;      // tank content at the moment of saving
    float stamina = 100.0f;     // grip reserve at the moment of saving
    std::vector<int> brokenWalls; // indices into the level's wall list
};

// Layout version of the save file. It is written into every file and
// checked on load: a file from an incompatible layout is refused instead of
// being misread, so bump this constant whenever the schema changes.
constexpr int SAVE_VERSION = 1;

namespace save_detail {

// Finds "key" in the text and parses the number that follows its colon.
// Tolerant of arbitrary whitespace; returns false when the key is absent.
inline bool extractNumber(const std::string& text, const std::string& key, float& out) {
    const std::string needle = "\"" + key + "\"";
    size_t pos = text.find(needle);
    if (pos == std::string::npos) return false;
    pos = text.find(':', pos + needle.size());
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
    if (pos >= text.size()) return false;
    char* end = nullptr;
    const float v = std::strtof(text.c_str() + pos, &end);
    if (end == text.c_str() + pos) return false;
    out = v;
    return true;
}

// Finds "key" in the text and parses the integer list inside the brackets
// that follow its colon. Stops cleanly at the closing bracket or at the
// first token that is not a number, so a hand-edited file cannot loop.
inline bool extractIntArray(const std::string& text, const std::string& key,
                            std::vector<int>& out) {
    const std::string needle = "\"" + key + "\"";
    size_t pos = text.find(needle);
    if (pos == std::string::npos) return false;
    pos = text.find('[', pos + needle.size());
    if (pos == std::string::npos) return false;
    ++pos;
    out.clear();
    while (pos < text.size()) {
        while (pos < text.size() &&
               (std::isspace(static_cast<unsigned char>(text[pos])) || text[pos] == ','))
            ++pos;
        if (pos >= text.size() || text[pos] == ']') break;
        char* end = nullptr;
        const long v = std::strtol(text.c_str() + pos, &end, 10);
        if (end == text.c_str() + pos) break;
        out.push_back(static_cast<int>(v));
        pos = static_cast<size_t>(end - text.c_str());
    }
    return true;
}

} // namespace save_detail

// Serializes the snapshot as a small versioned JSON document. The writer
// and the reader below are the only code that knows the exact layout.
inline bool writeSnapshot(const std::string& path, const GameSnapshot& s) {
    std::ofstream out(path);
    if (!out) return false;

    char head[256];
    std::snprintf(head, sizeof(head),
        "{\n"
        "  \"version\": %d,\n"
        "  \"player\": { \"x\": %.2f, \"y\": %.2f, \"facing\": %d, \"dash\": %d,"
        " \"oxygen\": %.2f, \"stamina\": %.2f },\n"
        "  \"brokenWalls\": [",
        SAVE_VERSION, s.playerX, s.playerY, s.facing,
        s.dashAvailable ? 1 : 0, s.oxygen, s.stamina);
    out << head;

    for (size_t i = 0; i < s.brokenWalls.size(); ++i) {
        if (i > 0) out << ", ";
        out << s.brokenWalls[i];
    }
    out << "]\n}\n";
    return static_cast<bool>(out);
}

// Reads a snapshot back. Missing fields keep the struct defaults, a missing
// or unreadable file and a version mismatch both report failure, so the
// caller can fall back to a fresh start.
inline bool readSnapshot(const std::string& path, GameSnapshot& s) {
    std::ifstream in(path);
    if (!in) return false;
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();

    float version = 0.0f;
    if (!save_detail::extractNumber(text, "version", version)) return false;
    if (static_cast<int>(version) != SAVE_VERSION) return false;

    save_detail::extractNumber(text, "x", s.playerX);
    save_detail::extractNumber(text, "y", s.playerY);
    float facing = 1.0f;
    if (save_detail::extractNumber(text, "facing", facing))
        s.facing = facing >= 0.0f ? 1 : -1;
    float dash = 1.0f;
    if (save_detail::extractNumber(text, "dash", dash))
        s.dashAvailable = dash != 0.0f;
    save_detail::extractNumber(text, "oxygen", s.oxygen);
    save_detail::extractNumber(text, "stamina", s.stamina);
    save_detail::extractIntArray(text, "brokenWalls", s.brokenWalls);
    return true;
}
