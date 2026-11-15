#pragma once

#include <SFML/Graphics.hpp>

#include "AssetManager.hpp"
#include "Display.hpp"
#include "State.hpp"

class Game
{
public:
    Game();

    void run();

private:

private:
    sf::RenderWindow window;
    sf::View gameView;
    AssetManager assets;
    StateStack stateStack;
};