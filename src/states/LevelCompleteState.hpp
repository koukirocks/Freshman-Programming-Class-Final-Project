#pragma once

#include "State.hpp"
#include "objects/Button.hpp"

#include <vector>
#include <string>
#include <array>

class LevelCompleteState : public State
{
private:
    sf::Text text;
    std::vector<Button> buttons;
    std::vector<sf::Text> starStandards;
    sf::RectangleShape panel;
    sf::Text timeLabel;
    int levelIndex;
    int stars;
    bool hasNextLevel;
    std::array<sf::Sprite, 3> filledStars;
    std::array<sf::Sprite, 3> emptyStars;

public:
    LevelCompleteState(
        AssetManager& assets,
        std::queue<StateRequest>& stateRequests,
        int levelIndex,
        float completionTime,
        int stars
    );
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt,sf::RenderWindow& window) override;
    void render(sf::RenderWindow& window) override;
};