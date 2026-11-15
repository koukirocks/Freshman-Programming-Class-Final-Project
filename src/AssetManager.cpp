#include "AssetManager.hpp"

#include <filesystem>
#include <stdexcept>

namespace
{
    std::filesystem::path resolveAssetPath(const std::string& path)
    {
        const std::filesystem::path requested(path);
        if (std::filesystem::exists(requested))
            return requested;

        const std::filesystem::path fromBuildDirectory =
            std::filesystem::path("..") / requested;
        if (std::filesystem::exists(fromBuildDirectory))
            return fromBuildDirectory;

        return requested;
    }
}

sf::Texture& AssetManager::getTexture(const std::string& path)
{
    auto it = textures.find(path);

    if (it != textures.end())
    {
        return *it->second;
    }

    auto texture = std::make_unique<sf::Texture>();

    const std::filesystem::path assetPath = resolveAssetPath(path);
    if (!texture->loadFromFile(assetPath.string()))
    {
        throw std::runtime_error(
            "Failed to load texture: " + path
        );
    }
    texture->setSmooth(false);

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

    const std::filesystem::path assetPath = resolveAssetPath(path);
    if (!font->openFromFile(assetPath.string()))
    {
        throw std::runtime_error(
            "Failed to load font: " + path
        );
    }

    sf::Font& reference = *font;

    fonts.emplace(path, std::move(font));

    return reference;
}