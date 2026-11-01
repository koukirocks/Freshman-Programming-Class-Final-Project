#include "Player.hpp"
#include "AssetManager.hpp"
#include <cstring>

Player::Player(AssetManager &assets)
: text(assets.getFont("assets/fonts/arial.ttf"))
{
    shape.setSize({50.f, 50.f});
    shape.setPosition({40.f, 30.f});
}

void Player::update(float dt)
{
    if (sf::Keyboard::isKeyPressed(
            sf::Keyboard::Key::A))
    {
        shape.move({-200.f * dt, 0.f});
    }

    if (sf::Keyboard::isKeyPressed(
            sf::Keyboard::Key::D))
    {
        shape.move({200.f * dt, 0.f});
    }

    if (sf::Keyboard::isKeyPressed(
            sf::Keyboard::Key::W))
    {
        shape.move({0.f, -200.f * dt});
    }

    if (sf::Keyboard::isKeyPressed(
            sf::Keyboard::Key::S))
    {
        shape.move({0.f, 200.f * dt});
    }
    sf::Vector2f position = shape.getPosition();
    text.setString(std::to_string(position.x)+","+std::to_string(position.y));
}

void Player::draw(sf::RenderWindow& window)
{
    window.draw(shape);
    window.draw(text);
}