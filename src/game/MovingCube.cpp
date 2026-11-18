#include "game/MovingCube.hpp"

#include <algorithm>
#include <cmath>

MovingCube::MovingCube(sf::Vector3f _position,sf::Vector3f _size,
    sf::Vector3f _travel,float _speed):
Cube(_position,_size),
startPosition(_position),
previousPosition(_position),
travel(_travel),
speed(std::max(_speed,0.f)),
distance(0.f),
direction(1.f),
travelLength(std::sqrt(
    _travel.x * _travel.x + _travel.y * _travel.y + _travel.z * _travel.z
))
{
}

void MovingCube::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
{
}

void MovingCube::update(float dt)
{
    previousPosition = getPosition();
    if (travelLength <= 0.f || speed <= 0.f)
        return;

    distance += direction * speed * std::max(dt,0.f);
    if (distance >= travelLength)
    {
        distance = travelLength;
        direction = -1.f;
    }
    else if (distance <= 0.f)
    {
        distance = 0.f;
        direction = 1.f;
    }

    const float fraction = distance / travelLength;
    setPosition(startPosition + travel * fraction);
}

sf::Vector3f MovingCube::getMovementDelta() const
{
    return getPosition() - previousPosition;
}

void MovingCube::render(sf::RenderWindow& window) const
{
}
