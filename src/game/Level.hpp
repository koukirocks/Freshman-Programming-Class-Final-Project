#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

struct LevelCubeData
{
    sf::Vector3f position;
    sf::Vector3f size;
};

struct LevelInstructionData
{
    std::string text;
    sf::Vector2f textPosition;
    unsigned int textSize;
    std::string buttonText;
    sf::Vector2f buttonPosition;
    sf::Vector2f buttonSize;
};

struct LevelData
{
    sf::Vector3f playerPosition;
    sf::Vector2f playerSize;
    std::vector<LevelCubeData> cubes;
    LevelCubeData finish;
    std::vector<LevelInstructionData> instructions;
    std::array<float, 3> starTimes;
};

std::vector<std::filesystem::path> getLevelFiles();
LevelData loadLevel(const std::filesystem::path& path);