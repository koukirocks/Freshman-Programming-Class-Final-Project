#include "Game.hpp"

Game::Game()
    : window(
        sf::VideoMode({1280, 720}),
        "My Game",
        sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize
    ),
    gameView(sf::FloatRect({0.f, 0.f}, Display::virtualResolution)),
    assets(),
    stateStack(
        assets
    )
{
    window.setFramerateLimit(60);
    Display::updateViewport(gameView, window.getSize());
    window.setView(gameView);
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
                Display::updateViewport(gameView, resized->size);
                window.setView(gameView);
            }
            stateStack.processEvent(event,window);
            if (event->is<sf::Event::Closed>())
            {   
                window.close();
            }
        }
        window.setView(gameView);
        stateStack.update(dt,window);
        window.clear();
        stateStack.render(window);
        window.display();
    }
}