#include "states/InstructState.hpp"

#include <SFML/Graphics.hpp>

#include <stdexcept>

InstructState::InstructState(
    AssetManager& assets,
    std::queue<StateRequest>& stateRequests,
    int levelIndex
):
State(assets,stateRequests),
assets(assets),
text(assets.getFont("assets/fonts/arial.ttf"),"",1),
currentInstruction(0)
{
    const auto levelFiles = getLevelFiles();
    if (levelIndex < 0 || levelIndex >= static_cast<int>(levelFiles.size()))
        throw std::out_of_range("Selected level does not exist");

    const LevelData level = loadLevel(levelFiles[levelIndex]);
    instructions = level.instructions;

    BG = sf::RectangleShape({2560,1440});
    BG.setFillColor(sf::Color(0,0,0,100));

    showInstruction();
}

void InstructState::showInstruction()
{
    const LevelInstructionData& instruction = instructions[currentInstruction];
    text.setString(instruction.text);
    text.setCharacterSize(instruction.textSize);
    text.setFillColor(sf::Color::White);
    text.setPosition(instruction.textPosition);

    buttons.clear();
    Button nextButton(
        sf::RectangleShape(),
        sf::Color(255,255,0),
        sf::Color(0,255,255),
        sf::Text(
            assets.getFont("assets/fonts/arial.ttf"),
            instruction.buttonText,
            instruction.textSize
        ),
        assets
    );
    nextButton.setPosition(instruction.buttonPosition);
    nextButton.setSize(instruction.buttonSize);
    nextButton.setTextColor(sf::Color(0,0,0));
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

        const auto mousePositioni = sf::Mouse::getPosition(window);
        const sf::Vector2f mousePosition(mousePositioni.x,mousePositioni.y);
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
    const auto mousePositioni = sf::Mouse::getPosition(window);
    const sf::Vector2f mousePosition(mousePositioni.x,mousePositioni.y);
    for (auto& button:buttons) button.update(mousePosition);
}

void InstructState::render(sf::RenderWindow& window)
{
    window.draw(BG);
    window.draw(text);
    for (auto& button:buttons) button.render(window);
}