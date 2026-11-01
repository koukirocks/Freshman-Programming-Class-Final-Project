#pragma once

#include "State.hpp"
#include "game/Level.hpp"
#include "objects/Button.hpp"

#include <memory>
#include <vector>

class PauseState : public State
{
private:
    sf::Text text;
    std::vector<Button> buttons;
    sf::RectangleShape BG;

public:
    PauseState(AssetManager& assets,std::queue<StateRequest>& stateRequests);
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt,sf::RenderWindow& window) override;
    void render(sf::RenderWindow& window) override;
};