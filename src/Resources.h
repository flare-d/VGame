#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>

// Cache of loaded texture files. Every texture is requested by its path
// (relative to the assets directory, resolved by the caller) and loaded
// exactly once; later requests return the cached instance. A failed load
// is cached as an empty texture so the game retries the file at most once
// per run and every caller sees the same "missing" result.
class ResourceCache {
public:
    const sf::Texture& texture(const std::string& path) {
        const auto it = textures.find(path);
        if (it != textures.end()) return it->second;
        sf::Texture tex;
        // The load result is consumed only to silence the nodiscard
        // attribute: a failure leaves the texture empty on purpose, which
        // the renderer detects as "sheet missing" and falls back to the
        // placeholder rectangle.
        const bool loaded = tex.loadFromFile(path);
        (void)loaded;
        const auto [slot, inserted] = textures.emplace(path, std::move(tex));
        (void)inserted;
        return slot->second;
    }

private:
    std::unordered_map<std::string, sf::Texture> textures;
};
