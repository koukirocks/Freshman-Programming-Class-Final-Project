#include "states/LevelSelectState.hpp"
#include "objects/Button.hpp"
#include "AssetManager.hpp"
#include "game/Level.hpp"
#include "UITheme.hpp"
#include "LevelProgress.hpp"

#include <SFML/Graphics.hpp>
#include <string>

LevelSelectState::LevelSelectState(AssetManager& assets,std::queue<StateRequest> &stateRequests):
State(assets,stateRequests)
{
    panel = UITheme::makePanel({24,20}, {1232,680});
    texts.push_back(UITheme::makeText(assets, "SELECT LEVEL", 36));
    UITheme::centerText(texts.back(), {640,80});

    Button backButton(
        sf::RectangleShape(),
        UITheme::accentHover,
        UITheme::accent,
        sf::Text(assets.getFont("assets/fonts/NotoSansSC.ttf"), "MAIN MENU", 20),
        assets
    );
    backButton.setPosition({150,80});
    backButton.setSize({150,44});
    backButton.setTextColor(UITheme::buttonText);
    navigationButtons.push_back(backButton);

    const auto levelFiles = getLevelFiles();
    for (std::size_t index = 0; index < levelFiles.size(); ++index)
    {
        sf::Text text(
            assets.getFont("assets/fonts/NotoSansSC.ttf"),
            std::to_string(index + 1),
            80
        );
        text.setFillColor(UITheme::buttonText);

        Button levelButton(
            sf::RectangleShape(),
            UITheme::accentHover,
            UITheme::accent,
            text,
            assets
        );
        levelButton.setPosition({350.f + 150.f * static_cast<float>(index % 4),
                                 185.f + 105.f * static_cast<float>(index / 4)});
        levelButton.setSize({72,64});
        buttons.push_back(levelButton);

        const sf::Texture& filledTexture =
            assets.getTexture("assets/images/star_filled.png");
        const sf::Texture& emptyTexture =
            assets.getTexture("assets/images/star_empty.png");
        std::array<sf::Sprite, 3> filledRow = {
            sf::Sprite(filledTexture),
            sf::Sprite(filledTexture),
            sf::Sprite(filledTexture)
        };
        std::array<sf::Sprite, 3> emptyRow = {
            sf::Sprite(emptyTexture),
            sf::Sprite(emptyTexture),
            sf::Sprite(emptyTexture)
        };
        const float centerX = 350.f + 150.f * static_cast<float>(index % 4);
        const float centerY = 235.f + 105.f * static_cast<float>(index / 4);
        for (std::size_t star = 0; star < 3; ++star)
        {
            const sf::Vector2f position{
                centerX - 36.f + 24.f * static_cast<float>(star),
                centerY - 12.f
            };
            filledRow[star].setPosition(position);
            emptyRow[star].setPosition(position);
            filledRow[star].setScale({0.375f, 0.375f});
            emptyRow[star].setScale({0.375f, 0.375f});
        }
        filledStars.push_back(std::move(filledRow));
        emptyStars.push_back(std::move(emptyRow));
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

        const sf::Vector2f mousePosition = Display::mousePosition(window);
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
            if (navigationButtons[0].isHovered(mousePosition))
            {
                stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
                stateRequests.push(StateRequest(StateRequest::Type::Push,StateID::Menu));
            }
        }
    }
}

void LevelSelectState::update(float dt,sf::RenderWindow& window)
{
    const sf::Vector2f mousePosition = Display::mousePosition(window);
    for (auto& button:buttons) button.update(mousePosition);
    for (auto& button:navigationButtons) button.update(mousePosition);
    for (std::size_t index = 0; index < filledStars.size(); ++index)
    {
        (void)LevelProgress::getStars(static_cast<int>(index));
    }
}

void LevelSelectState::render(sf::RenderWindow& window)
{
    const sf::View previousView = window.getView();
    sf::View menuView = window.getView();
    window.setView(menuView);

    window.draw(panel);
    for (auto& sprite:sprites) window.draw(sprite);
    for (auto& button:buttons) button.render(window);
    for (auto& button:navigationButtons) button.render(window);
    for (auto& text:texts) window.draw(text);
    for (std::size_t index = 0; index < filledStars.size(); ++index)
    {
        const int stars = LevelProgress::getStars(static_cast<int>(index));
        for (int star = 0; star < 3; ++star)
        {
            if (star < stars)
                window.draw(filledStars[index][star]);
            else
                window.draw(emptyStars[index][star]);
        }
    }

    window.setView(previousView);
}