#include "states/MenuState.hpp"
#include "objects/Button.hpp"
#include "AssetManager.hpp"
#include "UITheme.hpp"

#include <SFML/Graphics.hpp>

MenuState::MenuState(AssetManager& assets,std::queue<StateRequest> &stateRequests):
State(assets,stateRequests)
{
    Button playButton(
        sf::RectangleShape(),
        UITheme::accentHover,
        UITheme::accent,
        [&]{
            sf::Text t(
                assets.getFont("assets/fonts/NotoSansSC.ttf"),
                "Play",
                60
            );
            t.setFillColor(UITheme::buttonText);
            return t;
        }(),
        assets
    );
    playButton.setPosition({640,440});
    playButton.setSize({300,64});
    buttons.push_back(
        playButton
    );
    sf::Text title(
        assets.getFont("assets/fonts/NotoSansSC.ttf"),
        "2-3D PLATFORMER",
        52
    );
    title.setStyle(sf::Text::Bold);

    sf::FloatRect titleBounds = title.getLocalBounds();
    UITheme::centerText(title, {640,195});
    texts.push_back(
        title
    );
    panel = UITheme::makePanel({370,120}, {540,440});
    sf::Sprite background(assets.getTexture("assets/images/menu_background.png"));
    const sf::Vector2u textureSize = background.getTexture().getSize();
    background.setScale({
        Display::virtualResolution.x / static_cast<float>(textureSize.x),
        Display::virtualResolution.y / static_cast<float>(textureSize.y)
    });
    sprites.push_back(std::move(background));
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
    const sf::Vector2f mousePosition = Display::mousePosition(window);
    for (auto& button:buttons) button.update(mousePosition);
}

void MenuState::render(sf::RenderWindow& window)
{
    const sf::View previousView = window.getView();
    sf::View menuView = window.getView();
    window.setView(menuView);

    for (auto& sprite:sprites) window.draw(sprite);
    window.draw(panel);
    for (auto& button:buttons) button.render(window);
    for (auto& text:texts) window.draw(text);

    window.setView(previousView);
}