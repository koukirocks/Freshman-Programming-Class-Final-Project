#include "states/LevelCompleteState.hpp"

#include <SFML/Graphics.hpp>

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "game/Level.hpp"
#include "LevelProgress.hpp"
#include "UITheme.hpp"

namespace
{
    std::string formatTime(float seconds)
    {
        const int minutes = static_cast<int>(seconds) / 60;
        const int wholeSeconds = static_cast<int>(seconds) % 60;
        const int centiseconds = static_cast<int>(seconds * 100.f) % 100;

        std::ostringstream result;
        result << std::setfill('0') << std::setw(2) << minutes
               << ':' << std::setw(2) << wholeSeconds
               << '.' << std::setw(2) << centiseconds;
        return result.str();
    }

    std::string formatStandard(float seconds)
    {
        const int totalSeconds = static_cast<int>(seconds);
        const int minutes = totalSeconds / 60;
        const int remainingSeconds = totalSeconds % 60;

        if (minutes == 0)
            return std::to_string(remainingSeconds) + "s";
        if (remainingSeconds == 0)
            return std::to_string(minutes) + "m";
        return std::to_string(minutes) + "m"
            + std::to_string(remainingSeconds) + "s";
    }

    Button makeButton(
        AssetManager& assets,
        const std::string& label,
        sf::Vector2f position
    )
    {
        Button button(
            sf::RectangleShape(),
            UITheme::accentHover,
            UITheme::accent,
            sf::Text(assets.getFont("assets/fonts/NotoSansSC.ttf"), label, 20),
            assets
        );
        button.setPosition(position);
        button.setSize({240,56});
        button.setTextColor(UITheme::buttonText);
        return button;
    }
}

LevelCompleteState::LevelCompleteState(
    AssetManager& assets,
    std::queue<StateRequest>& stateRequests,
    int _levelIndex,
    float completionTime,
    int _stars
):
State(assets,stateRequests),
text(assets.getFont("assets/fonts/NotoSansSC.ttf"),"LEVEL COMPLETE",40),
timeLabel(assets.getFont("assets/fonts/NotoSansSC.ttf"),"",32),
levelIndex(_levelIndex),
stars(_stars),
hasNextLevel(false),
filledStars{
    sf::Sprite(assets.getTexture("assets/images/star_filled.png")),
    sf::Sprite(assets.getTexture("assets/images/star_filled.png")),
    sf::Sprite(assets.getTexture("assets/images/star_filled.png"))
},
emptyStars{
    sf::Sprite(assets.getTexture("assets/images/star_empty.png")),
    sf::Sprite(assets.getTexture("assets/images/star_empty.png")),
    sf::Sprite(assets.getTexture("assets/images/star_empty.png"))
}
{
    const auto levelFiles = getLevelFiles();
    if (levelIndex < 0 || levelIndex >= static_cast<int>(levelFiles.size()))
        throw std::out_of_range("Completed level does not exist");
    hasNextLevel = levelIndex + 1 < static_cast<int>(levelFiles.size());

    const LevelData level = loadLevel(levelFiles[levelIndex]);

    text.setStyle(sf::Text::Bold);
    timeLabel.setStyle(sf::Text::Bold);
    text.setFillColor(UITheme::text);
    UITheme::centerText(text, {640,115});

    timeLabel.setString("TIME  " + formatTime(completionTime));
    timeLabel.setFillColor(UITheme::accent);
    UITheme::centerText(timeLabel, {640,175});

    sf::Text bestTimeLabel(
        assets.getFont("assets/fonts/NotoSansSC.ttf"),
        "BEST  " + formatTime(LevelProgress::getBestTime(levelIndex)),
        22
    );
    bestTimeLabel.setStyle(sf::Text::Bold);
    bestTimeLabel.setFillColor(UITheme::mutedText);
    UITheme::centerText(bestTimeLabel, {640,205});
    starStandards.push_back(std::move(bestTimeLabel));

    for (std::size_t star = 0; star < 3; ++star)
    {
        const sf::Vector2f center{
            640.f + 72.f * (static_cast<float>(star) - 1.f),
            265.f
        };
        filledStars[star].setOrigin({32.f, 32.f});
        emptyStars[star].setOrigin({32.f, 32.f});
        filledStars[star].setPosition(center);
        emptyStars[star].setPosition(center);
        filledStars[star].setScale({0.8f, 0.8f});
        emptyStars[star].setScale({0.8f, 0.8f});

        sf::Text standard(
            assets.getFont("assets/fonts/NotoSansSC.ttf"),
            formatStandard(level.starTimes[2 - star]),
            16
        );
        standard.setStyle(sf::Text::Bold);
        standard.setFillColor(UITheme::mutedText);
        UITheme::centerText(standard, {center.x, 314.f});
        starStandards.push_back(std::move(standard));
    }

    panel = UITheme::makePanel({80,24}, {1120,672});
    if (hasNextLevel)
    {
        buttons.push_back(makeButton(assets, "NEXT LEVEL", {640,385}));
        buttons.push_back(makeButton(assets, "RETRY", {640,465}));
        buttons.push_back(makeButton(assets, "LEVEL SELECT", {640,545}));
        buttons.push_back(makeButton(assets, "MAIN MENU", {640,625}));
    }
    else
    {
        buttons.push_back(makeButton(assets, "RETRY", {640,385}));
        buttons.push_back(makeButton(assets, "LEVEL SELECT", {640,465}));
        buttons.push_back(makeButton(assets, "MAIN MENU", {640,545}));
    }
}

void LevelCompleteState::processEvent(
    const std::optional<sf::Event>& event,
    sf::RenderWindow& window
)
{
    if (!event.has_value())
        return;

    if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
    {
        if (mousePressed->button != sf::Mouse::Button::Left)
            return;

        const sf::Vector2f mousePosition = Display::mousePosition(window);
        if (hasNextLevel && buttons[0].isHovered(mousePosition))
        {
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(
                StateRequest::Type::Push,
                StateID::Level,
                levelIndex + 1
            ));
        }
        else if (buttons[hasNextLevel ? 1 : 0].isHovered(mousePosition))
        {
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(
                StateRequest::Type::Push,
                StateID::Level,
                levelIndex
            ));
        }
        else if (buttons[hasNextLevel ? 2 : 1].isHovered(mousePosition))
        {
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(StateRequest::Type::Push,StateID::LevelSelect));
        }
        else if (buttons[hasNextLevel ? 3 : 2].isHovered(mousePosition))
        {
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(StateRequest::Type::Push,StateID::Menu));
        }
    }
}

void LevelCompleteState::update(float dt,sf::RenderWindow& window)
{
    const sf::Vector2f mousePosition = Display::mousePosition(window);
    for (auto& button:buttons)
        button.update(mousePosition);
}

void LevelCompleteState::render(sf::RenderWindow& window)
{
    window.draw(panel);
    window.draw(text);
    window.draw(timeLabel);
    for (int star = 0; star < 3; ++star)
    {
        if (star < stars)
            window.draw(filledStars[star]);
        else
            window.draw(emptyStars[star]);
    }
    for (auto& standard:starStandards)
        window.draw(standard);
    for (auto& button:buttons)
        button.render(window);
}
