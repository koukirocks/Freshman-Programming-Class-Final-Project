#pragma once

#include <SFML/Graphics.hpp>

namespace Display
{
    inline constexpr sf::Vector2f virtualResolution{1280.f, 720.f};
    inline constexpr sf::Vector2f virtualCenter{640.f, 360.f};

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

    inline sf::View makeView(sf::Vector2u windowSize, sf::Vector2f center)
    {
        sf::View view(sf::FloatRect({0.f, 0.f}, virtualResolution));
        view.setCenter(center);
        updateViewport(view, windowSize);
        return view;
    }

    inline void setView(sf::RenderWindow& window, sf::View& view)
    {
        updateViewport(view, window.getSize());
        window.setView(view);
    }

    inline void setView(sf::RenderWindow& window, sf::Vector2f center)
    {
        window.setView(makeView(window.getSize(),center));
    }
}
