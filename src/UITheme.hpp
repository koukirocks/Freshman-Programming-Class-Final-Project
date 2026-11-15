#pragma once

#include "AssetManager.hpp"

#include <SFML/Graphics.hpp>

namespace UITheme
{
    inline constexpr sf::Color background{8, 17, 31};
    inline constexpr sf::Color panel{15, 31, 52, 235};
    inline constexpr sf::Color panelBorder{76, 201, 240};
    inline constexpr sf::Color accent{76, 201, 240};
    inline constexpr sf::Color accentHover{128, 232, 255};
    inline constexpr sf::Color text{235, 244, 255};
    inline constexpr sf::Color mutedText{157, 181, 207};
    inline constexpr sf::Color buttonText{8, 17, 31};

    inline sf::Text makeText(AssetManager& assets, const std::string& value,
                             unsigned int size, sf::Color color = text)
    {
        sf::Text result(assets.getFont("assets/fonts/NotoSansSC.ttf"), value, size);
        result.setFillColor(color);
        result.setStyle(sf::Text::Bold);
        return result;
    }

    inline void centerText(sf::Text& value, sf::Vector2f center)
    {
        const sf::FloatRect bounds = value.getLocalBounds();
        value.setOrigin({
            bounds.position.x + bounds.size.x / 2.f,
            bounds.position.y + bounds.size.y / 2.f
        });
        value.setPosition(center);
    }

    inline sf::RectangleShape makePanel(sf::Vector2f position, sf::Vector2f size)
    {
        sf::RectangleShape result(size);
        result.setPosition(position);
        result.setFillColor(panel);
        result.setOutlineColor(panelBorder);
        result.setOutlineThickness(2.f);
        return result;
    }
}
