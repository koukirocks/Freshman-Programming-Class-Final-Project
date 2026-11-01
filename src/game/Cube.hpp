#pragma once

#include "AssetManager.hpp"
#include "game/GameObject.hpp"

#include <SFML/Graphics.hpp>

class Cube : public GameObject 
{
public:
    virtual ~Cube() = default;

    Cube(sf::Vector3f _position,sf::Vector3f _size);

    virtual void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) = 0;
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderWindow& window) const = 0;

    sf::Vector3f getSize() const;
    void setSize(sf::Vector3f _size);

protected:
    sf::Vector3f size;
};