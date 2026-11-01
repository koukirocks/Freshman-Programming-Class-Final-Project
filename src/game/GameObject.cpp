#include "game/GameObject.hpp"
#include <SFML/Graphics.hpp>

sf::Vector3f GameObject::getPosition() const 
{
    return position;
}

void GameObject::setPosition(sf::Vector3f _position) 
{
    position=_position;
}