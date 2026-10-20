#include "KeyBindings.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

namespace {

// Removes leading and trailing whitespace from a string.
std::string trim(const std::string& s) {
    const size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Converts a string to lowercase ASCII so key and action names match case-insensitively.
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Converts a keyboard key to a short human-readable name used in the config file.
std::string keyToName(sf::Keyboard::Key key) {
    const int k = static_cast<int>(key);

    // Letters, digits and function keys form contiguous enum ranges.
    if (key >= sf::Keyboard::Key::A && key <= sf::Keyboard::Key::Z)
        return std::string(1, static_cast<char>('A' + (k - static_cast<int>(sf::Keyboard::Key::A))));
    if (key >= sf::Keyboard::Key::Num0 && key <= sf::Keyboard::Key::Num9)
        return std::string(1, static_cast<char>('0' + (k - static_cast<int>(sf::Keyboard::Key::Num0))));
    if (key >= sf::Keyboard::Key::F1 && key <= sf::Keyboard::Key::F12)
        return "F" + std::to_string(1 + k - static_cast<int>(sf::Keyboard::Key::F1));

    static const std::unordered_map<sf::Keyboard::Key, std::string> names = {
        {sf::Keyboard::Key::Left, "Left"},     {sf::Keyboard::Key::Right, "Right"},
        {sf::Keyboard::Key::Up, "Up"},         {sf::Keyboard::Key::Down, "Down"},
        {sf::Keyboard::Key::Space, "Space"},   {sf::Keyboard::Key::Escape, "Escape"},
        {sf::Keyboard::Key::Enter, "Enter"},   {sf::Keyboard::Key::Tab, "Tab"},
        {sf::Keyboard::Key::LShift, "LShift"}, {sf::Keyboard::Key::RShift, "RShift"},
        {sf::Keyboard::Key::LControl, "LControl"}, {sf::Keyboard::Key::RControl, "RControl"},
        {sf::Keyboard::Key::LAlt, "LAlt"},     {sf::Keyboard::Key::RAlt, "RAlt"},
    };
    const auto it = names.find(key);
    if (it != names.end()) return it->second;

    // Keys outside the table are stored by raw numeric value so no binding is ever lost.
    return "#" + std::to_string(k);
}

// Parses a key name produced by keyToName, or a plain letter / digit / F-key name.
// Returns sf::Keyboard::Key::Unknown when the name cannot be resolved.
sf::Keyboard::Key nameToKey(const std::string& rawName) {
    const std::string name = toLower(trim(rawName));
    if (name.empty()) return sf::Keyboard::Key::Unknown;

    if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'z')
        return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::A) + (name[0] - 'a'));
    if (name.size() == 1 && name[0] >= '0' && name[0] <= '9')
        return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::Num0) + (name[0] - '0'));
    if (name.size() >= 2 && name[0] == 'f') {
        const int n = std::atoi(name.c_str() + 1);
        if (n >= 1 && n <= 12)
            return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::F1) + (n - 1));
    }

    static const std::unordered_map<std::string, sf::Keyboard::Key> byName = {
        {"left", sf::Keyboard::Key::Left},         {"right", sf::Keyboard::Key::Right},
        {"up", sf::Keyboard::Key::Up},             {"down", sf::Keyboard::Key::Down},
        {"space", sf::Keyboard::Key::Space},       {"escape", sf::Keyboard::Key::Escape},
        {"enter", sf::Keyboard::Key::Enter},       {"tab", sf::Keyboard::Key::Tab},
        {"lshift", sf::Keyboard::Key::LShift},     {"rshift", sf::Keyboard::Key::RShift},
        {"lcontrol", sf::Keyboard::Key::LControl}, {"rcontrol", sf::Keyboard::Key::RControl},
        {"lalt", sf::Keyboard::Key::LAlt},         {"ralt", sf::Keyboard::Key::RAlt},
    };
    const auto it = byName.find(name);
    if (it != byName.end()) return it->second;

    if (name[0] == '#') {
        const int k = std::atoi(name.c_str() + 1);
        if (k >= 0) return static_cast<sf::Keyboard::Key>(k);
    }
    return sf::Keyboard::Key::Unknown;
}

// Returns the action whose config file name matches the given text,
// or Action::Count when the name matches no action.
Action actionFromName(const std::string& rawName) {
    const std::string name = toLower(trim(rawName));
    if (name == "left")      return Action::Left;
    if (name == "right")     return Action::Right;
    if (name == "up")        return Action::Up;
    if (name == "down")      return Action::Down;
    if (name == "jump")      return Action::Jump;
    if (name == "grab")      return Action::Grab;
    if (name == "run")       return Action::Run;
    if (name == "dash")      return Action::Dash;
    if (name == "debug")     return Action::ToggleDebug;
    if (name == "debughurt") return Action::DebugHurt;
    if (name == "save")      return Action::Save;
    if (name == "load")      return Action::Load;
    if (name == "quit")      return Action::Quit;
    return Action::Count;
}

// Returns the config file name of an action.
const char* actionName(Action action) {
    switch (action) {
        case Action::Left:        return "left";
        case Action::Right:       return "right";
        case Action::Up:          return "up";
        case Action::Down:        return "down";
        case Action::Jump:        return "jump";
        case Action::Grab:        return "grab";
        case Action::Run:         return "run";
        case Action::Dash:        return "dash";
        case Action::ToggleDebug: return "debug";
        case Action::DebugHurt:   return "debughurt";
        case Action::Save:        return "save";
        case Action::Load:        return "load";
        case Action::Quit:        return "quit";
        default:                  return "unknown";
    }
}

} // namespace

KeyBindings::KeyBindings() {
    keys[static_cast<int>(Action::Left)]        = sf::Keyboard::Key::Left;
    keys[static_cast<int>(Action::Right)]       = sf::Keyboard::Key::Right;
    keys[static_cast<int>(Action::Up)]          = sf::Keyboard::Key::Up;
    keys[static_cast<int>(Action::Down)]        = sf::Keyboard::Key::Down;
    keys[static_cast<int>(Action::Jump)]        = sf::Keyboard::Key::Z;
    keys[static_cast<int>(Action::Grab)]        = sf::Keyboard::Key::V;
    keys[static_cast<int>(Action::Run)]         = sf::Keyboard::Key::LShift;
    keys[static_cast<int>(Action::Dash)]        = sf::Keyboard::Key::X;
    keys[static_cast<int>(Action::ToggleDebug)] = sf::Keyboard::Key::F3;
    keys[static_cast<int>(Action::DebugHurt)]   = sf::Keyboard::Key::F4;
    keys[static_cast<int>(Action::Save)]        = sf::Keyboard::Key::F5;
    keys[static_cast<int>(Action::Load)]        = sf::Keyboard::Key::F9;
    keys[static_cast<int>(Action::Quit)]        = sf::Keyboard::Key::Escape;
}

sf::Keyboard::Key KeyBindings::get(Action action) const {
    return keys[static_cast<int>(action)];
}

void KeyBindings::rebind(Action action, sf::Keyboard::Key key) {
    keys[static_cast<int>(action)] = key;
}

bool KeyBindings::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false; // a missing file is fine: the default bindings stay active

    std::string line;
    while (std::getline(in, line)) {
        const std::string clean = trim(line);
        if (clean.empty() || clean[0] == '#') continue; // skip blank lines and comment lines

        const size_t eq = clean.find('=');
        if (eq == std::string::npos) continue; // lines without '=' carry no binding

        const Action action = actionFromName(clean.substr(0, eq));
        if (action == Action::Count) continue;

        const sf::Keyboard::Key key = nameToKey(clean.substr(eq + 1));
        if (key == sf::Keyboard::Key::Unknown) continue;

        rebind(action, key);
    }
    return true;
}

bool KeyBindings::saveToFile(const std::string& path) const {
    std::ofstream out(path);
    if (!out) return false;

    out << "# Key bindings. One entry per line: action = key name.\n";
    out << "# Available actions: left, right, up, down, jump, grab, run, dash,\n";
    out << "# debug, debughurt, save, load, quit.\n";
    out << "# Key names: letters (A-Z), digits (0-9), F1-F12, Left, Right, Up, Down,\n";
    out << "# Space, Escape, Enter, Tab, LShift, RShift, LControl, RControl, LAlt, RAlt.\n";
    for (int i = 0; i < static_cast<int>(Action::Count); ++i) {
        const Action action = static_cast<Action>(i);
        out << actionName(action) << " = " << keyToName(get(action)) << '\n';
    }
    return true;
}
