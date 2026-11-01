#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <string>
#include <unordered_map>

class AssetManager
{
public:
    sf::Texture& getTexture(const std::string& path);
    sf::Font& getFont(const std::string& path);

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> fonts;
};