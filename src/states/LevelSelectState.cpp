#include "states/LevelSelectState.hpp"
#include "objects/Button.hpp"
#include "AssetManager.hpp"
#include "game/Level.hpp"

#include <SFML/Graphics.hpp>
#include <string>

LevelSelectState::LevelSelectState(AssetManager& assets,std::queue<StateRequest> &stateRequests):
State(assets,stateRequests)
{
    const auto levelFiles = getLevelFiles();
    for (std::size_t index = 0; index < levelFiles.size(); ++index)
    {
        sf::Text text(
            assets.getFont("assets/fonts/arial.ttf"),
            std::to_string(index + 1),
            80
        );
        text.setFillColor(sf::Color(0,0,0));

        Button levelButton(
            sf::RectangleShape(),
            sf::Color(255,255,0),
            sf::Color(0,255,255),
            text,
            assets
        );
        levelButton.setPosition({300.f + 150.f * static_cast<float>(index % 8),
                                 360.f + 150.f * static_cast<float>(index / 8)});
        levelButton.setSize({100,100});
        buttons.push_back(levelButton);
    }
}

void LevelSelectState::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
{
    if (!event.has_value())
        return;

    if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button != sf::Mouse::Button::Left)
            return;

        const auto mousePositioni = sf::Mouse::getPosition(window);
        const sf::Vector2f mousePosition(mousePositioni.x,mousePositioni.y);
        for (std::size_t index = 0; index < buttons.size(); ++index)
        {
            if (buttons[index].isHovered(mousePosition))
            {
                stateRequests.push(
                    StateRequest(StateRequest::Type::Pop,StateID::Null,index)
                );                stateRequests.push(
                    StateRequest(StateRequest::Type::Push,StateID::Level,index)
                );
                return;
            }
        }
    }
}

void LevelSelectState::update(float dt,sf::RenderWindow& window)
{
    auto mousePositioni = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosition(mousePositioni.x,mousePositioni.y);
    for (auto& button:buttons) button.update(mousePosition);
}

void LevelSelectState::render(sf::RenderWindow& window)
{
    const sf::View previousView = window.getView();
    sf::View menuView = window.getDefaultView();
    window.setView(menuView);

    for (auto& sprite:sprites) window.draw(sprite);
    for (auto& button:buttons) button.render(window);
    for (auto& text:texts) window.draw(text);

    window.setView(previousView);
}