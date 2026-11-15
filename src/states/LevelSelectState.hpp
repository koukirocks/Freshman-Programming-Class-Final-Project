#pragma once

#include "State.hpp"
#include "objects/Button.hpp"
#include <vector>
#include <queue>
#include <array>

class LevelSelectState : public State
{
public:
    std::vector<Button> buttons;
    std::vector<Button> navigationButtons;
    std::vector<sf::Text> texts;
    std::vector<std::array<sf::Sprite, 3>> filledStars;
    std::vector<std::array<sf::Sprite, 3>> emptyStars;
    std::vector<sf::Sprite> sprites;
    sf::RectangleShape panel;
public:
    LevelSelectState(AssetManager& assets,std::queue<StateRequest> &stateRequests);
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt,sf::RenderWindow& window) override;
    void render(sf::RenderWindow& window) override;
};
