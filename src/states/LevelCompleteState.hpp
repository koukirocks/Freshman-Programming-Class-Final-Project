#pragma once

#include "State.hpp"
#include "objects/Button.hpp"

#include <vector>

class LevelCompleteState : public State
{
private:
    sf::Text text;
    std::vector<Button> buttons;

public:
    LevelCompleteState(AssetManager& assets,std::queue<StateRequest>& stateRequests);
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt,sf::RenderWindow& window) override;
    void render(sf::RenderWindow& window) override;
};