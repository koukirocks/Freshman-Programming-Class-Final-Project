#pragma once

#include <SFML/Graphics.hpp>

namespace Display
{
    inline constexpr sf::Vector2f virtualResolution{1280.f, 720.f};

    inline sf::Vector2f mousePosition(const sf::RenderWindow& window)
    {
        return window.mapPixelToCoords(sf::Mouse::getPosition(window));
    }

    inline void updateViewport(sf::View& view, sf::Vector2u windowSize)
    {
        if (windowSize.x == 0 || windowSize.y == 0)
            return;

        const float windowAspect = static_cast<float>(windowSize.x)
            / static_cast<float>(windowSize.y);
        const float virtualAspect = virtualResolution.x / virtualResolution.y;

        sf::FloatRect viewport({0.f, 0.f}, {1.f, 1.f});
        if (windowAspect > virtualAspect)
        {
            const float width = virtualAspect / windowAspect;
            viewport.position.x = (1.f - width) / 2.f;
            viewport.size.x = width;
        }
        else if (windowAspect < virtualAspect)
        {
            const float height = windowAspect / virtualAspect;
            viewport.position.y = (1.f - height) / 2.f;
            viewport.size.y = height;
        }

        view.setViewport(viewport);
    }
}
