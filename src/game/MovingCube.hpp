#pragma once

#include "game/Cube.hpp"

class MovingCube : public Cube
{
public:
    MovingCube(sf::Vector3f _position,sf::Vector3f _size,
        sf::Vector3f _travel,float _speed);

    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) const override;

    sf::Vector3f getMovementDelta() const;

private:
    sf::Vector3f startPosition;
    sf::Vector3f previousPosition;
    sf::Vector3f travel;
    float speed;
    float distance;
    float direction;
    float travelLength;
};
