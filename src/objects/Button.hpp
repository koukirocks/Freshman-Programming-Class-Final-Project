#pragma once

#include "AssetManager.hpp"

#include <SFML/Graphics.hpp>
#include <string>
#include <variant>

class Button
{
private:
    std::variant<sf::RectangleShape, sf::CircleShape> shape;
    sf::Color hoveredColor, normalColor;
    sf::Text text;
    bool hovered;

public:
    Button(
        std::variant<sf::RectangleShape, sf::CircleShape> _shape,
        sf::Color _hoveredColor,
        sf::Color _normalColor,
        sf::Text _text,
        AssetManager& assets);

    bool isHovered(sf::Vector2f mousePosition);
    bool isHovered() const;
    void setPosition(sf::Vector2f position);
    void setSize(sf::Vector2f position);
    void setRadius(float radius);
    void setButtonColor(sf::Color color);
    void setTextColor(sf::Color color);
    void update(sf::Vector2f mousePosition);
    void render(sf::RenderWindow& window);
};