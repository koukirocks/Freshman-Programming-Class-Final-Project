#include "AssetManager.hpp"

#include <stdexcept>

sf::Texture& AssetManager::getTexture(const std::string& path)
{
    auto it = textures.find(path);

    if (it != textures.end())
    {
        return *it->second;
    }

    auto texture = std::make_unique<sf::Texture>();

    if (!texture->loadFromFile(path))
    {
        throw std::runtime_error(
            "Failed to load texture: " + path
        );
    }

    sf::Texture& reference = *texture;

    textures.emplace(path, std::move(texture));

    return reference;
}

sf::Font& AssetManager::getFont(const std::string& path)
{
    auto it = fonts.find(path);

    if (it != fonts.end())
    {
        return *it->second;
    }

    auto font = std::make_unique<sf::Font>();

    if (!font->openFromFile(path))
    {
        throw std::runtime_error(
            "Failed to load font: " + path
        );
    }

    sf::Font& reference = *font;

    fonts.emplace(path, std::move(font));

    return reference;
}