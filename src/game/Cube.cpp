#include "game/Cube.hpp"
#include <SFML/Graphics.hpp>

Cube::Cube(sf::Vector3f _position,sf::Vector3f _size):
size(_size)
{
    setPosition(_position);
}

sf::Vector3f Cube::getSize() const
{
    return size;
}

void Cube::setSize(sf::Vector3f _size) 
{
    size=_size;
}