#pragma once

#include <SFML/Graphics.hpp>
#include "AssetManager.hpp"

class Player
{
public:
    Player(AssetManager &assets);

    void update(float dt);
    void draw(sf::RenderWindow& window);

private:
    sf::RectangleShape shape;
    sf::Text text;
};