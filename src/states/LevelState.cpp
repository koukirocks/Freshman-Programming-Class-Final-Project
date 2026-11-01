#include "states/LevelState.hpp"

#include "game/ObstacleCube.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

constexpr float edgeMargin = 100.f; // pixels from the window edge before the world starts scrolling

namespace
{
    sf::Vector2f forwardAxis(float angle)
    {
        return {std::cos(angle),std::sin(angle)};
    }

    float dot(sf::Vector2f first,sf::Vector2f second)
    {
        return first.x * second.x + first.y * second.y;
    }

    void addPoint(std::vector<sf::Vector2f>& points,sf::Vector2f point)
    {
        for (const sf::Vector2f existing:points)
        {
            if (std::abs(existing.x - point.x) < 0.01f
                && std::abs(existing.y - point.y) < 0.01f)
            {
                return;
            }
        }
        points.push_back(point);
    }

    std::vector<sf::Vector2f> cubeSlice(
        const Cube& cube,
        sf::Vector3f playerPosition,
        float playerAngle
    )
    {
        const sf::Vector3f cubePosition = cube.getPosition();
        const sf::Vector3f cubeSize = cube.getSize();
        const sf::Vector2f forward = forwardAxis(playerAngle);
        const sf::Vector2f normal(-forward.y,forward.x);
        const std::array<sf::Vector3f,8> vertices = {
            sf::Vector3f(cubePosition.x,cubePosition.y,cubePosition.z),
            sf::Vector3f(cubePosition.x + cubeSize.x,cubePosition.y,cubePosition.z),
            sf::Vector3f(cubePosition.x,cubePosition.y + cubeSize.y,cubePosition.z),
            sf::Vector3f(cubePosition.x + cubeSize.x,cubePosition.y + cubeSize.y,cubePosition.z),
            sf::Vector3f(cubePosition.x,cubePosition.y,cubePosition.z + cubeSize.z),
            sf::Vector3f(cubePosition.x + cubeSize.x,cubePosition.y,cubePosition.z + cubeSize.z),
            sf::Vector3f(cubePosition.x,cubePosition.y + cubeSize.y,cubePosition.z + cubeSize.z),
            sf::Vector3f(cubePosition.x + cubeSize.x,cubePosition.y + cubeSize.y,cubePosition.z + cubeSize.z)
        };
        const std::array<std::array<int,2>,12> edges = {{
            {{0,1}},{{0,2}},{{1,3}},{{2,3}},
            {{4,5}},{{4,6}},{{5,7}},{{6,7}},
            {{0,4}},{{1,5}},{{2,6}},{{3,7}}
        }};
        std::vector<sf::Vector2f> points;

        for (const auto& edge:edges)
        {
            const sf::Vector3f first = vertices[edge[0]];
            const sf::Vector3f second = vertices[edge[1]];
            const float firstDistance = dot(normal,{first.x - playerPosition.x,
                                                    first.y - playerPosition.y});
            const float secondDistance = dot(normal,{second.x - playerPosition.x,
                                                     second.y - playerPosition.y});

            if (std::abs(firstDistance) < 0.01f)
            {
                addPoint(points,{dot(forward,{first.x - playerPosition.x,
                                              first.y - playerPosition.y}),
                                 first.z - playerPosition.z});
            }
            if (firstDistance * secondDistance < 0)
            {
                const float fraction = firstDistance / (firstDistance - secondDistance);
                const sf::Vector3f intersection = first + (second - first) * fraction;
                addPoint(points,{dot(forward,{intersection.x - playerPosition.x,
                                              intersection.y - playerPosition.y}),
                                 intersection.z - playerPosition.z});
            }
        }

        const sf::Vector2f center = [&] {
            sf::Vector2f result(0,0);
            for (const sf::Vector2f point:points)
                result += point;
            if (!points.empty())
                result /= static_cast<float>(points.size());
            return result;
        }();
        std::sort(points.begin(),points.end(),[&](sf::Vector2f first,sf::Vector2f second) {
            return std::atan2(first.y - center.y,first.x - center.x)
                < std::atan2(second.y - center.y,second.x - center.x);
        });
        return points;
    }

    void renderCubeSlice(
        sf::RenderWindow& window,
        const Cube& cube,
        sf::Vector3f playerPosition,
        float playerAngle,
        sf::Vector2f screenOffset,
        sf::Color color
    )
    {
        const std::vector<sf::Vector2f> points = cubeSlice(cube,playerPosition,playerAngle);
        if (points.size() < 3)
            return;

        const sf::Vector2f screenCenter = window.getDefaultView().getCenter();
        sf::ConvexShape shape(points.size());
        for (std::size_t index = 0; index < points.size(); ++index)
        {
            shape.setPoint(index,{screenCenter.x + screenOffset.x + points[index].x,
                                  screenCenter.y - screenOffset.y - points[index].y});
        }
        shape.setFillColor(color);
        window.draw(shape);
    }
}

LevelState::LevelState(AssetManager& assets,std::queue<StateRequest>& stateRequests,int levelIndex):
State(assets,stateRequests),
level(),
cubes(),
finishCube(),
player({0,0,0},{0,0}),
cameraPosition({0,0,0}),
cameraAngle(0),
cameraInitialized(false),
levelIndex(levelIndex),
instructionsShown(false)
{
    const auto levelFiles = getLevelFiles();
    if (levelIndex < 0 || levelIndex >= static_cast<int>(levelFiles.size()))
        throw std::out_of_range("Selected level does not exist");

    level = loadLevel(levelFiles[levelIndex]);
    player = Player(level.playerPosition,level.playerSize);
    finishCube = std::make_unique<FinishCube>(level.finish.position,level.finish.size);

    for (const auto& cube:level.cubes)
    {
        cubes.push_back(
            std::make_unique<ObstacleCube>(cube.position,cube.size)
        );
    }
}

void LevelState::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
{
    player.processEvent(event,window);
    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->scancode == sf::Keyboard::Scan::Escape)
        {
            stateRequests.push(
                StateRequest(StateRequest::Type::Push,StateID::Pause)
            );
        }
    }
}

void LevelState::update(float dt,sf::RenderWindow& window)
{
    player.update(dt,cubes);

    if (!cameraInitialized)
    {
        // World (0,0,0) at the bottom-left of the window.
        // cameraPosition is the world point drawn at the window center.
        const sf::Vector2f viewSize = window.getDefaultView().getSize();
        cameraAngle = player.getAngle();
        const sf::Vector2f forward = forwardAxis(cameraAngle);

        // Forward axis: world x (at angle 0) sits viewSize.x/2 to the right of the left edge.
        // The y component doesn't affect the screen at angle 0; use the player's y so the
        // camera lies in the player's slice plane.
        cameraPosition = {
            forward.x * (viewSize.x / 2.f),
            player.getPosition().y,
            viewSize.y / 2.f
        };
        cameraInitialized = true;
    }

    if (cameraInitialized && !instructionsShown)
    {
        instructionsShown = true;
        stateRequests.push(
            StateRequest(StateRequest::Type::Push,StateID::Instruct,levelIndex)
        );
    }

    const sf::Vector3f playerPosition = player.getPosition();
    const float newAngle = player.getAngle();

    // Rotation: keep the player's on-screen position unchanged.
    if (std::abs(newAngle - cameraAngle) > 0.0001f)
    {
        const sf::Vector2f oldForward = forwardAxis(cameraAngle);
        const float screenX = dot(oldForward,{playerPosition.x - cameraPosition.x,
                                              playerPosition.y - cameraPosition.y});

        const sf::Vector2f newForward = forwardAxis(newAngle);
        cameraPosition.x = playerPosition.x - newForward.x * screenX;
        cameraPosition.y = playerPosition.y - newForward.y * screenX;
        cameraAngle = newAngle;
    }

    // Edge scrolling: if the player is within edgeMargin of a window edge,
    // move the camera (and so the world) instead of letting the player go further.
    const sf::Vector2f forward = forwardAxis(cameraAngle);
    const sf::Vector2f viewSize = window.getDefaultView().getSize();
    const sf::Vector2f playerSize = player.getSize();

    // Player's offset from the screen center (x right, y up).
    const float offsetX = dot(forward,{playerPosition.x - cameraPosition.x,
                                       playerPosition.y - cameraPosition.y});
    const float offsetY = playerPosition.z - cameraPosition.z;

    // Allowed range for that offset, so the player rectangle stays inside the margins.
    const float minX = -viewSize.x / 2.f + edgeMargin;
    const float maxX =  viewSize.x / 2.f - edgeMargin - playerSize.x;
    const float minY = -viewSize.y / 2.f + edgeMargin;
    const float maxY =  viewSize.y / 2.f - edgeMargin - playerSize.y;

    // If the window is too small for the margins, fall back to keeping the player centered.
    const float clampedX = (minX <= maxX) ? std::clamp(offsetX,minX,maxX) : (minX + maxX) / 2.f;
    const float clampedY = (minY <= maxY) ? std::clamp(offsetY,minY,maxY) : (minY + maxY) / 2.f;

    // Shift the camera by the overshoot. Moving it along `forward` by d reduces offsetX by d.
    const float shiftX = offsetX - clampedX;
    cameraPosition.x += forward.x * shiftX;
    cameraPosition.y += forward.y * shiftX;
    cameraPosition.z += offsetY - clampedY;

    if (player.collidesWith(*finishCube))
    {
        stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
        stateRequests.push(StateRequest(StateRequest::Type::Push,StateID::LevelComplete));
    }
}

void LevelState::render(sf::RenderWindow& window)
{
    const sf::Vector3f playerPosition = player.getPosition();
    const float playerAngle = player.getAngle();
    const sf::Vector2f forward = forwardAxis(playerAngle);
    const sf::Vector2f playerDisplacement(
        playerPosition.x - cameraPosition.x,
        playerPosition.y - cameraPosition.y
    );
    const sf::Vector2f screenOffset(
        dot(forward,playerDisplacement),
        playerPosition.z - cameraPosition.z
    );
    for (const auto& cube:cubes)
    {
        renderCubeSlice(window,*cube,playerPosition,playerAngle,screenOffset,sf::Color(70,110,180));
    }

    renderCubeSlice(window,*finishCube,playerPosition,playerAngle,screenOffset,sf::Color(255,210,40));

    const sf::Vector2f playerSize = player.getSize();
    sf::RectangleShape playerShape({playerSize.x,playerSize.y});
    const sf::Vector2f screenCenter = window.getDefaultView().getCenter();
    playerShape.setPosition({screenCenter.x + screenOffset.x,
                              screenCenter.y - screenOffset.y - playerSize.y});
    playerShape.setFillColor(sf::Color(240,70,70));
    window.draw(playerShape);
}