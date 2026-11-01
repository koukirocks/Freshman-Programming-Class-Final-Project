#include "objects/Button.hpp"

#include <SFML/Graphics.hpp>

#include <string>
#include <variant>
#include <stdexcept>

Button::Button(
    std::variant<sf::RectangleShape, sf::CircleShape> _shape,
    sf::Color _hoveredColor,
    sf::Color _normalColor,
    sf::Text _text,
    AssetManager& assets):
    shape(_shape),
    hoveredColor(_hoveredColor),
    normalColor(_normalColor),
    text(_text)
{
    ;
}

struct SisHovered {
    sf::Vector2f mousePosition;
    bool operator()(const sf::RectangleShape& shape)
	{
        return shape.getGlobalBounds().contains(mousePosition);
	}
    bool operator()(const sf::CircleShape& shape)
	{
		sf::Vector2f local =
            shape.getInverseTransform().transformPoint(mousePosition);

        float radius = shape.getRadius();

        return local.x * local.x + local.y * local.y
                <= radius * radius;
	}
};

bool Button::isHovered(sf::Vector2f mousePosition)
{
    hovered = std::visit(
        SisHovered{mousePosition},
        shape
    );
    return hovered;
}

bool Button::isHovered() const
{
    return hovered;
}

void Button::setPosition(sf::Vector2f position)
{
    std::visit(
        [&](auto& shape) {
            shape.setPosition(position);
        },
        shape
    );
    
    sf::FloatRect bounds = std::visit(
        [&](auto& shape) {
            return shape.getGlobalBounds();
        },
        shape
    );

    sf::FloatRect textBounds = text.getLocalBounds();

    text.setOrigin({
        textBounds.position.x + textBounds.size.x / 2.f,
        textBounds.position.y + textBounds.size.y / 2.f
    });

    text.setPosition({
        bounds.position.x + bounds.size.x / 2.f,
        bounds.position.y + bounds.size.y / 2.f
    });
}


struct SsetSize {
    sf::Vector2f size;
    void operator()(sf::RectangleShape& shape)
	{
		shape.setSize(size);
        shape.setOrigin({size.x/2.f,size.y/2.f});
	}
    void operator()(sf::CircleShape& shape)
	{
		throw std::invalid_argument("Circle tries to setSize");
	}
};
void Button::setSize(sf::Vector2f size)
{
    std::visit(
        SsetSize{size},
        shape
    );
}

struct SsetRadius {
    float radius;
    void operator()(sf::RectangleShape& shape)
	{
		throw std::invalid_argument("Rectangle tries to setRadius");
	}
    void operator()(sf::CircleShape& shape)
	{
		shape.setRadius(radius);
        shape.setOrigin({radius,radius});
	}
};
void Button::setRadius(float radius)
{
    std::visit(
        SsetRadius{radius},
        shape
    );
}

void Button::setButtonColor(sf::Color color)
{
    std::visit(
        [&](auto& shape) {
            shape.setFillColor(color);
        },
        shape
    );
}

void Button::setTextColor(sf::Color color)
{
    text.setFillColor(color);
}

void Button::update(sf::Vector2f mousePosition)
{
    if (isHovered(mousePosition)) {
        setButtonColor(hoveredColor);
    } else {
        setButtonColor(normalColor);
    }
}

void Button::render(sf::RenderWindow& window)
{
    std::visit(
        [&](const auto& shape) {
            window.draw(shape);
        },
        shape
    );
    window.draw(text);
}