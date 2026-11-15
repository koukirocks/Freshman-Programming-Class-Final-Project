#include "states/PauseState.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

#include <SFML/Graphics.hpp>
#include "UITheme.hpp"

PauseState::PauseState(AssetManager& assets,std::queue<StateRequest>& stateRequests):
State(assets,stateRequests),
text(assets.getFont("assets/fonts/NotoSansSC.ttf"),"PAUSED",42)
{
    text.setStyle(sf::Text::Bold);
text.setFillColor(UITheme::text);
UITheme::centerText(text, {640,115});
panel = UITheme::makePanel({80,24}, {1120,672});

    Button ResumeButton(
        sf::RectangleShape(),
        UITheme::accentHover,
        UITheme::accent,
        sf::Text(assets.getFont("assets/fonts/NotoSansSC.ttf"),"RESUME",28),
        assets
    );
    ResumeButton.setPosition({640,220});
    ResumeButton.setSize({240,56});
    ResumeButton.setTextColor(UITheme::buttonText);
    buttons.push_back(ResumeButton);

    Button levelSelectButton(
        sf::RectangleShape(), UITheme::accentHover, UITheme::accent,
        sf::Text(assets.getFont("assets/fonts/NotoSansSC.ttf"), "LEVEL SELECT", 20), assets
    );
    levelSelectButton.setPosition({640,300});
    levelSelectButton.setSize({240,56});
    levelSelectButton.setTextColor(UITheme::buttonText);
    buttons.push_back(levelSelectButton);

    Button mainMenuButton(
        sf::RectangleShape(), UITheme::accentHover, UITheme::accent,
        sf::Text(assets.getFont("assets/fonts/NotoSansSC.ttf"), "MAIN MENU", 20), assets
    );
    mainMenuButton.setPosition({640,380});
    mainMenuButton.setSize({240,56});
    mainMenuButton.setTextColor(UITheme::buttonText);
    buttons.push_back(mainMenuButton);

    BG = sf::RectangleShape(Display::virtualResolution);
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

        const sf::Vector2f mousePosition = Display::mousePosition(window);
        if (buttons[0].isHovered(mousePosition))
        {
            stateRequests.push(
                StateRequest(StateRequest::Type::Pop,StateID::Null)
            );
        }
        else if (buttons[1].isHovered(mousePosition))
        {
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(StateRequest::Type::Push,StateID::LevelSelect));
        }
        else if (buttons[2].isHovered(mousePosition))
        {
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(StateRequest::Type::Push,StateID::Menu));
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
    const sf::Vector2f mousePosition = Display::mousePosition(window);
    for (auto& button:buttons) button.update(mousePosition);
}

void PauseState::render(sf::RenderWindow& window)
{
    window.draw(BG);
    window.draw(panel);
    window.draw(text);
    for (auto& button:buttons) button.render(window);
}