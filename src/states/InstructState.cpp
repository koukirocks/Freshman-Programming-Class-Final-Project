#include "states/InstructState.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include "UITheme.hpp"

#include <stdexcept>

InstructState::InstructState(
    AssetManager& assets,
    std::queue<StateRequest>& stateRequests,
    int levelIndex
):
State(assets,stateRequests),
assets(assets),
text(assets.getFont("assets/fonts/NotoSansSC.ttf"),"",1),
currentInstruction(0)
{
    const auto levelFiles = getLevelFiles();
    if (levelIndex < 0 || levelIndex >= static_cast<int>(levelFiles.size()))
        throw std::out_of_range("Selected level does not exist");

    const LevelData level = loadLevel(levelFiles[levelIndex]);
    instructions = level.instructions;

    BG = sf::RectangleShape(Display::virtualResolution);
    BG.setFillColor(sf::Color(0,0,0,100));
    panel = UITheme::makePanel({24,20}, {1232,680});

    showInstruction();
}

void InstructState::showInstruction()
{
    const LevelInstructionData& instruction = instructions[currentInstruction];
    text.setString(sf::String::fromUtf8(
        instruction.text.begin(),
        instruction.text.end()
    ));
    text.setCharacterSize(instruction.textSize);
    text.setFillColor(UITheme::text);
    text.setStyle(sf::Text::Bold);
    text.setPosition(instruction.textPosition);

    buttons.clear();
    Button nextButton(
        sf::RectangleShape(),
        UITheme::accentHover,
        UITheme::accent,
        sf::Text(
            assets.getFont("assets/fonts/NotoSansSC.ttf"),
            instruction.buttonText,
            std::min(instruction.textSize, 30u)
        ),
        assets
    );
    nextButton.setPosition(instruction.buttonPosition);
    nextButton.setSize(instruction.buttonSize);
    nextButton.setTextColor(UITheme::buttonText);
    buttons.push_back(nextButton);
}

void InstructState::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
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
            if (++currentInstruction < instructions.size())
                showInstruction();
            else
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

void InstructState::update(float dt,sf::RenderWindow& window)
{
    const sf::Vector2f mousePosition = Display::mousePosition(window);
    for (auto& button:buttons) button.update(mousePosition);
}

void InstructState::render(sf::RenderWindow& window)
{
    window.draw(BG);
    window.draw(panel);
    window.draw(text);
    for (auto& button:buttons) button.render(window);
}