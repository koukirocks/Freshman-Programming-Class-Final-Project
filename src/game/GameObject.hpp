#pragma once

#include "AssetManager.hpp"

#include <SFML/Graphics.hpp>

class GameObject
{
public:
    virtual ~GameObject() = default;

    virtual void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) = 0;
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderWindow& window) const = 0;

    sf::Vector3f getPosition() const;
    void setPosition(sf::Vector3f position);

protected:
    sf::Vector3f position;
};