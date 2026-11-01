#include "states/LevelCompleteState.hpp"

#include <SFML/Graphics.hpp>

LevelCompleteState::LevelCompleteState(AssetManager& assets,std::queue<StateRequest>& stateRequests):
State(assets,stateRequests),
text(assets.getFont("assets/fonts/arial.ttf"),"Level Complete",80)
{
    text.setFillColor(sf::Color::White);
    text.setPosition({100,100});

    Button levelSelectButton(
        sf::RectangleShape(),
        sf::Color(255,255,0),
        sf::Color(0,255,255),
        sf::Text(assets.getFont("assets/fonts/arial.ttf"),"Level Select",60),
        assets
    );
    levelSelectButton.setPosition({1280,360});
    levelSelectButton.setSize({500,150});
    levelSelectButton.setTextColor(sf::Color(0,0,0));
    buttons.push_back(levelSelectButton);
}

void LevelCompleteState::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
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
            stateRequests.push(
                StateRequest(StateRequest::Type::Push,StateID::LevelSelect)
            );
        }
    }
}

void LevelCompleteState::update(float dt,sf::RenderWindow& window)
{
    const auto mousePositioni = sf::Mouse::getPosition(window);
    const sf::Vector2f mousePosition(mousePositioni.x,mousePositioni.y);
    for (auto& button:buttons) button.update(mousePosition);
}

void LevelCompleteState::render(sf::RenderWindow& window)
{
    window.draw(text);
    for (auto& button:buttons) button.render(window);
}