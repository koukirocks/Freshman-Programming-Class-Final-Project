#include "Game.hpp"

#include <iostream>

int main()
{
    try
    {
        Game game;
        game.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Game startup failed: " << error.what() << '\n';
        return 1;
    }

    return 0;
}