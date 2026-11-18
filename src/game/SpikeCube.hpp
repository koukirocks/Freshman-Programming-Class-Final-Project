#pragma once

#include "game/Cube.hpp"

class SpikeCube : public Cube
{
public:
    SpikeCube(sf::Vector3f _position,sf::Vector3f _size);

    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) const override;
};
