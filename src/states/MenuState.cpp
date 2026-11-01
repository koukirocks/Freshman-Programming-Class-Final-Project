#include "states/MenuState.hpp"
#include "objects/Button.hpp"
#include "AssetManager.hpp"

#include <SFML/Graphics.hpp>

MenuState::MenuState(AssetManager& assets,std::queue<StateRequest> &stateRequests):
State(assets,stateRequests)
{
    Button playButton(
        sf::RectangleShape(),
        sf::Color(255,255,0),
        sf::Color(0,255,255),
        [&]{
            sf::Text t(
                assets.getFont("assets/fonts/arial.ttf"),
                "Play",
                60
            );
            t.setFillColor(sf::Color(0,0,0));
            return t;
        }(),
        assets
    );
    playButton.setPosition({1280,360*3});
    playButton.setSize({300,150});
    buttons.push_back(
        playButton
    );
    sf::Text title(
        assets.getFont("assets/fonts/arial.ttf"),
        "2-3D Platformer",
        100
    );

    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setOrigin(
        titleBounds.size / 2.f
    );
    title.setPosition({1280,360});
    texts.push_back(
        title
    );
    sprites.push_back(
        sf::Sprite(
            assets.getTexture("assets/images/menu_background.png")
        )
    );
}

void MenuState::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
{
    if (!event)
        return;

    if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button == sf::Mouse::Button::Left)
        {
            if (buttons[0].isHovered()) {
                stateRequests.push(
                    StateRequest(StateRequest::Type::Pop,StateID::Null)
                );
                stateRequests.push(
                    StateRequest(StateRequest::Type::Push,StateID::LevelSelect)
                );       
            }
        }
    }
}

void MenuState::update(float dt,sf::RenderWindow& window)
{
    auto mousePositioni = sf::Mouse::getPosition(window);
    sf::Vector2f mousePosition(mousePositioni.x,mousePositioni.y);
    for (auto& button:buttons) button.update(mousePosition);
}

void MenuState::render(sf::RenderWindow& window)
{
    const sf::View previousView = window.getView();
    sf::View menuView = window.getDefaultView();
    window.setView(menuView);

    for (auto& sprite:sprites) window.draw(sprite);
    for (auto& button:buttons) button.render(window);
    for (auto& text:texts) window.draw(text);

    window.setView(previousView);
}