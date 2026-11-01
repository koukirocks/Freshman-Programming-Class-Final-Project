#include "Game.hpp"

Game::Game()
    : window(
        sf::VideoMode({2560, 1440}),
        "My Game",
        sf::Style::Titlebar | sf::Style::Close
    ),
    assets(),
    stateStack(
        assets
    )
{
    window.setFramerateLimit(60);
    stateStack.push(stateStack.createState(StateID::Menu));
}

void Game::run()
{
    sf::Clock clock;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();
        while (const std::optional event = window.pollEvent())
        {
            stateStack.processEvent(event,window);
            if (event->is<sf::Event::Closed>())
            {   
                window.close();
            }
        }
        stateStack.update(dt,window);
        window.clear();
        stateStack.render(window);
        window.display();
    }
}