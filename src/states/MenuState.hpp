#pragma once

#include "State.hpp"
#include "objects/Button.hpp"
#include <vector>
#include <queue>

class MenuState : public State
{
public:
    std::vector<Button> buttons;
    std::vector<sf::Text> texts;
    std::vector<sf::Sprite> sprites;
public:
    MenuState(AssetManager& assets,std::queue<StateRequest> &stateRequests);
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt,sf::RenderWindow& window) override;
    void render(sf::RenderWindow& window) override;
};

