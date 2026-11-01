#include "states/PauseState.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

#include <SFML/Graphics.hpp>

PauseState::PauseState(AssetManager& assets,std::queue<StateRequest>& stateRequests):
State(assets,stateRequests),
text(assets.getFont("assets/fonts/arial.ttf"),"Pause",80)
{
    text.setFillColor(sf::Color::White);
    text.setPosition({100,100});

    Button ResumeButton(
        sf::RectangleShape(),
        sf::Color(255,255,0),
        sf::Color(0,255,255),
        sf::Text(assets.getFont("assets/fonts/arial.ttf"),"Resume",60),
        assets
    );
    ResumeButton.setPosition({1280,360});
    ResumeButton.setSize({500,150});
    ResumeButton.setTextColor(sf::Color(0,0,0));
    buttons.push_back(ResumeButton);

    BG = sf::RectangleShape({2560,1440});
    BG.setFillColor(sf::Color(0,0,0,100));
    
}

void PauseState::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
{
    if (!event.has_value())
        return;

    if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button != sf::Mouse::Button::Left)
            return;

        const auto mousePositioni = sf::Mouse::getPosition(window);
        const sf::Vector2f mousePosition(mousePositioni.x,mousePositioni.y);
        if (buttons[0].isHovered(mousePosition))
        {
            stateRequests.push(
                StateRequest(StateRequest::Type::Pop,StateID::Null)
            );
        }
    }
    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->scancode == sf::Keyboard::Scan::Escape)
        {
            stateRequests.push(
                StateRequest(StateRequest::Type::Pop,StateID::Null)
            );
        }
    }
}

void PauseState::update(float dt,sf::RenderWindow& window)
{
    const auto mousePositioni = sf::Mouse::getPosition(window);
    const sf::Vector2f mousePosition(mousePositioni.x,mousePositioni.y);
    for (auto& button:buttons) button.update(mousePosition);
}

void PauseState::render(sf::RenderWindow& window)
{
    window.draw(BG);
    window.draw(text);
    for (auto& button:buttons) button.render(window);
}