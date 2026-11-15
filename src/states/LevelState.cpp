#include "states/LevelState.hpp"

#include "game/ObstacleCube.hpp"
#include "UITheme.hpp"
#include "LevelProgress.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

constexpr float edgeMargin = 100.f; // pixels from the window edge before the world starts scrolling
constexpr float cameraMoveSpeed = 500.f;
constexpr float cameraFollowResponsiveness = 12.f;

namespace
{
    std::string formatTime(float seconds)
    {
        const int minutes = static_cast<int>(seconds) / 60;
        const int wholeSeconds = static_cast<int>(seconds) % 60;
        const int centiseconds = static_cast<int>(seconds * 100.f) % 100;

        std::ostringstream result;
        result << std::setfill('0') << std::setw(2) << minutes
               << ':' << std::setw(2) << wholeSeconds
               << '.' << std::setw(2) << centiseconds;
        return result.str();
    }

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

        const sf::Vector2f screenCenter = window.getView().getCenter();
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
,
restartRequested(false)
,
deathPending(false),
deathElapsed(0.f),
fadeInElapsed(0.f),
cameraFollowPending(false),
levelTime(0.f),
earnedStars(0),
timerPanel(UITheme::makePanel({24.f, 24.f}, {190.f, 64.f})),
timerText(UITheme::makeText(assets, "TIME  00:00.00", 24, UITheme::text))
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
        if (keyPressed->scancode == sf::Keyboard::Scan::R)
        {
            deathPending = true;
            deathElapsed = 1.f;
            restartRequested = true;
            return;
        }
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
    if (deathPending)
    {
        constexpr float deathDelay = 1.f;
        constexpr float fadeDuration = 0.35f;
        constexpr float blackHoldDuration = 2.f;
        deathElapsed += std::max(dt, 0.f);
        if (deathElapsed >= deathDelay + fadeDuration + blackHoldDuration)
        {
            stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
            stateRequests.push(StateRequest(StateRequest::Type::Push,StateID::Level,levelIndex));
            restartRequested = true;
            return;
        }
        return;
    }

    fadeInElapsed += std::max(dt, 0.f);
    if (instructionsShown)
        levelTime += std::max(dt, 0.f);
    player.update(dt,cubes);

    if (!restartRequested && player.getPosition().z < -500.f)
    {
        deathPending = true;
        deathElapsed = 0.f;
        return;
    }

    if (!cameraInitialized)
    {
        // Start with the player near the center of the view. The camera must
        // use the player's position because levels may start away from origin.
        const sf::Vector2f viewSize = window.getView().getSize();
        cameraAngle = player.getAngle();

        cameraPosition = {
            player.getPosition().x,
            player.getPosition().y,
            player.getPosition().z + viewSize.y / 4.f
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
    const bool playerIsMoving =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E);
    const bool cameraIsMoving =
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);

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

    const sf::Vector2f forward = forwardAxis(cameraAngle);

    // Edge scrolling: if the player is within edgeMargin of a window edge,
    // move the camera (and so the world) instead of letting the player go further.
    const sf::Vector2f viewSize = window.getView().getSize();
    const sf::Vector2f playerSize = player.getSize();

    // Allowed range for that offset, so the player rectangle stays inside the margins.
    const float minX = -viewSize.x / 2.f + edgeMargin;
    const float maxX =  viewSize.x / 2.f - edgeMargin - playerSize.x;
    const float minY = -viewSize.y / 2.f + edgeMargin;
    const float maxY =  viewSize.y / 2.f - edgeMargin - playerSize.y;

    if (cameraIsMoving)
    {
        float horizontalMovement = 0.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            horizontalMovement -= cameraMoveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            horizontalMovement += cameraMoveSpeed * dt;

        cameraPosition.x += forward.x * horizontalMovement;
        cameraPosition.y += forward.y * horizontalMovement;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            cameraPosition.z += cameraMoveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            cameraPosition.z -= cameraMoveSpeed * dt;
    }

    if (playerIsMoving)
        cameraFollowPending = true;

    if (cameraFollowPending)
    {
        // Player movement restores the normal camera-follow behavior smoothly.
        const float currentOffsetX = dot(forward,{playerPosition.x - cameraPosition.x,
                                                  playerPosition.y - cameraPosition.y});
        const float currentOffsetY = playerPosition.z - cameraPosition.z;
        const float followOffsetX = (minX <= maxX)
            ? std::clamp(currentOffsetX,minX,maxX)
            : (minX + maxX) / 2.f;
        const float followOffsetY = (minY <= maxY)
            ? std::clamp(currentOffsetY,minY,maxY)
            : (minY + maxY) / 2.f;
        const float shiftX = currentOffsetX - followOffsetX;
        const sf::Vector3f targetCameraPosition = cameraPosition
            + sf::Vector3f(
                forward.x * shiftX,
                forward.y * shiftX,
                currentOffsetY - followOffsetY
            );
        const float followAmount = 1.f
            - std::exp(-cameraFollowResponsiveness * std::max(dt, 0.f));
        cameraPosition += (targetCameraPosition - cameraPosition) * followAmount;

        const sf::Vector3f remainingOffset(
            targetCameraPosition.x - cameraPosition.x,
            targetCameraPosition.y - cameraPosition.y,
            targetCameraPosition.z - cameraPosition.z
        );
        if (std::abs(remainingOffset.x) < 0.1f
            && std::abs(remainingOffset.y) < 0.1f
            && std::abs(remainingOffset.z) < 0.1f)
        {
            cameraPosition = targetCameraPosition;
            cameraFollowPending = false;
        }
    }

    // Let the camera follow below world z = 0, but stop it after a short fall.
    constexpr float lowestCameraWorldZ = -300.f;
    cameraPosition.z = std::max(
        cameraPosition.z,
        viewSize.y / 2.f + lowestCameraWorldZ
    );

    // Camera panning may move the player off-screen without killing them.
    // Death is based on the player's world position instead.
    constexpr float deathWorldZ = -300.f;
    const bool playerHasFallen =
        playerPosition.z + playerSize.y < deathWorldZ;
    if (!restartRequested && playerHasFallen)
    {
        deathPending = true;
        deathElapsed = 0.f;
        return;
    }

    if (!restartRequested && player.collidesWith(*finishCube))
    {
        earnedStars = levelTime <= level.starTimes[0] ? 3
            : levelTime <= level.starTimes[1] ? 2
            : levelTime <= level.starTimes[2] ? 1
            : 0;
        LevelProgress::recordStars(levelIndex, earnedStars);
        LevelProgress::recordTime(levelIndex, levelTime);
        stateRequests.push(StateRequest(StateRequest::Type::Pop,StateID::Null));
        stateRequests.push(StateRequest(
            StateRequest::Type::Push,
            StateID::LevelComplete,
            levelIndex,
            levelTime,
            earnedStars
        ));
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
    const sf::Vector2f screenCenter = window.getView().getCenter();
    playerShape.setPosition({screenCenter.x + screenOffset.x,
                              screenCenter.y - screenOffset.y - playerSize.y});
    playerShape.setFillColor(sf::Color(240,70,70));
    window.draw(playerShape);

    timerText.setString("TIME  " + formatTime(levelTime));
    timerText.setPosition({42.f, 38.f});
    window.draw(timerPanel);
    window.draw(timerText);

    if (deathPending)
    {
        constexpr float deathDelay = 1.f;
        constexpr float fadeDuration = 0.35f;
        const float fadeProgress = std::clamp(
            (deathElapsed - deathDelay) / fadeDuration,
            0.f,
            1.f
        );
        const float alpha = fadeProgress >= 1.f
            ? 255.f
            : 255.f * fadeProgress;
        sf::RectangleShape fade(Display::virtualResolution);
        fade.setFillColor(sf::Color(0,0,0,
            static_cast<std::uint8_t>(alpha)));
        window.draw(fade);
    }
    else
    {
        constexpr float fadeInDuration = 0.5f;
        const float fadeProgress = std::clamp(
            fadeInElapsed / fadeInDuration,
            0.f,
            1.f
        );
        sf::RectangleShape fade(Display::virtualResolution);
        fade.setFillColor(sf::Color(0,0,0,
            static_cast<std::uint8_t>(255.f * (1.f - fadeProgress))));
        window.draw(fade);
    }
}