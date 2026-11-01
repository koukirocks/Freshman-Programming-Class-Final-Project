#pragma once

#include "AssetManager.hpp"
#include "game/GameObject.hpp"
#include "game/Cube.hpp"

#include <SFML/Graphics.hpp>
#include <vector>

class Player : public GameObject
{
public:
    virtual ~Player() = default;
    
    Player(sf::Vector3f _position,sf::Vector2f _size);
    
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window);
    void update(float dt) override;
    void update(float dt,const std::vector<std::unique_ptr<Cube>>& cubes);
    void render(sf::RenderWindow& window) const;

    sf::Vector2f getSize() const;
    float getAngle() const;
    bool collidesWith(const Cube& cube) const;

protected:
    float angle;
    sf::Vector2f size;
    bool isOnGround;
    float upSpeed,forwardSpeed;
    float upAcceleration,forwardAcceleration;
    float gravity;
    float jumpStrength;
    float moveSpeed;
    float rotationSpeed;
};