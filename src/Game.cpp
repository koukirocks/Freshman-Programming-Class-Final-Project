#include "Game.hpp"

Game::Game()
    : window(
        sf::VideoMode({1280, 720}),
        "My Game",
        sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize
    ),
    assets(),
    stateStack(
        assets
    )
{
    window.setFramerateLimit(60);
    Display::setView(window,Display::virtualCenter);
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
            if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                Display::setView(window,Display::virtualCenter);
            }
            stateStack.processEvent(event,window);
            if (event->is<sf::Event::Closed>())
            {   
                window.close();
            }
        }
        Display::setView(window,Display::virtualCenter);
        stateStack.update(dt,window);
        window.clear();
        stateStack.render(window);
        window.display();
    }
}