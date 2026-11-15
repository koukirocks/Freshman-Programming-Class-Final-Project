#pragma once

#include "State.hpp"
#include "game/Level.hpp"
#include "objects/Button.hpp"

#include <memory>
#include <vector>

class InstructState : public State
{
private:
    AssetManager& assets;
    sf::Text text;
    std::vector<Button> buttons;
    std::vector<LevelInstructionData> instructions;
    std::size_t currentInstruction;
    sf::RectangleShape BG;
    sf::RectangleShape panel;

    void showInstruction();

public:
    InstructState(
        AssetManager& assets,
        std::queue<StateRequest>& stateRequests,
        int levelIndex
    );
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt,sf::RenderWindow& window) override;
    void render(sf::RenderWindow& window) override;
};