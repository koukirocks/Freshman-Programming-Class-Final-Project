#include "states/LevelState.hpp"

#include "game/ObstacleCube.hpp"
#include "game/MovingCube.hpp"
#include "game/SpikeCube.hpp"
#include "UITheme.hpp"
#include "LevelProgress.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>

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

    std::string formatAngle(float radians)
    {
        constexpr float pi = 3.14159265358979323846f;
        float degrees = std::fmod(radians * 180.f / pi, 360.f);
        if (degrees < 0.f)
            degrees += 360.f;

        std::ostringstream result;
        result << std::fixed << std::setprecision(1) << degrees;
        return result.str();
    }

    sf::ConvexShape makeAngleIndicator()
    {
        sf::ConvexShape result(7);
        result.setPoint(0,{0.f,1.f});
        result.setPoint(1,{28.f,1.f});
        result.setPoint(2,{28.f,0.f});
        result.setPoint(3,{36.f,3.f});
        result.setPoint(4,{28.f,6.f});
        result.setPoint(5,{28.f,5.f});
        result.setPoint(6,{0.f,5.f});
        result.setOrigin({0.f,3.f});
        result.setFillColor(UITheme::accent);
        return result;
    }

    sf::Vector2f forwardAxis(float angle)
    {
        return {std::cos(angle),std::sin(angle)};
    }

    float dot(sf::Vector2f first,sf::Vector2f second)
    {
        return first.x * second.x + first.y * second.y;
    }

    struct CubeSlice
    {
        float start;
        float end;
        float height;
    };

    float forwardPosition(sf::Vector3f position, sf::Vector2f forward)
    {
        return dot(forward,{position.x,position.y});
    }

    std::optional<CubeSlice> cubeSlice(
        const Cube& cube,
        sf::Vector3f playerPosition,
        float playerAngle
    )
    {
        const sf::Vector3f cubePosition = cube.getPosition();
        const sf::Vector3f cubeSize = cube.getSize();
        const sf::Vector2f forward = forwardAxis(playerAngle);
        const float origin[2] = {playerPosition.x,playerPosition.y};
        const float direction[2] = {forward.x,forward.y};
        const float minimum[2] = {cubePosition.x,cubePosition.y};
        const float maximum[2] = {
            cubePosition.x + cubeSize.x,
            cubePosition.y + cubeSize.y
        };

        float start = -std::numeric_limits<float>::infinity();
        float end = std::numeric_limits<float>::infinity();
        for (int axis = 0; axis < 2; ++axis)
        {
            if (std::abs(direction[axis]) < 1e-6f)
            {
                if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis])
                    return std::nullopt;
                continue;
            }

            float first = (minimum[axis] - origin[axis]) / direction[axis];
            float second = (maximum[axis] - origin[axis]) / direction[axis];
            if (first > second)
                std::swap(first,second);
            start = std::max(start,first);
            end = std::min(end,second);
        }

        if (start > end)
            return std::nullopt;

        return CubeSlice{
            start,
            end,
            cubeSize.z
        };
    }

    void renderCubeSlice(
        sf::RenderWindow& window,
        const Cube& cube,
        sf::Vector3f playerPosition,
        float playerAngle,
        sf::Color color
    )
    {
        const std::optional<CubeSlice> slice = cubeSlice(cube,playerPosition,playerAngle);
        if (!slice || slice->end <= slice->start)
            return;

        sf::RectangleShape shape({
            slice->end - slice->start,
            slice->height
        });
        const sf::Vector2f forward = forwardAxis(playerAngle);
        shape.setPosition({
            forwardPosition(playerPosition,forward) + slice->start,
            -(cube.getPosition().z + cube.getSize().z)
        });
        shape.setFillColor(color);
        window.draw(shape);
    }
}

LevelState::LevelState(AssetManager& assets,std::queue<StateRequest>& stateRequests,int levelIndex):
State(assets,stateRequests),
level(),
cubes(),
spikes(),
finishCube(),
player({0,0,0},{0,0}),
cameraPosition({0,0,0}),
cameraAngle(0),
worldView(sf::FloatRect({0.f, 0.f}, Display::virtualResolution)),
cameraInitialized(false),
levelIndex(levelIndex),
instructionsShown(false)
,
restartRequested(false)
,
deathPending(false),
shakeOnDeath(false),
deathElapsed(0.f),
fadeInElapsed(0.f),
cameraFollowPending(false),
levelTime(0.f),
earnedStars(0),
timerPanel(UITheme::makePanel({24.f, 24.f}, {190.f, 64.f})),
timerText(UITheme::makeText(assets, "TIME  00:00.00", 24, UITheme::text)),
anglePanel(UITheme::makePanel({230.f, 24.f}, {190.f, 96.f})),
angleText(UITheme::makeText(assets, "ANGLE  0.0 deg", 16, UITheme::text)),
angleIndicator(makeAngleIndicator())
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
    for (const auto& cube:level.movingCubes)
    {
        cubes.push_back(
            std::make_unique<MovingCube>(
                cube.position,cube.size,cube.travel,cube.speed
            )
        );
    }
    for (const auto& spike:level.spikes)
        spikes.push_back(std::make_unique<SpikeCube>(spike.position,spike.size));
}

void LevelState::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
{
    player.processEvent(event,window);
    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->scancode == sf::Keyboard::Scan::R)
        {
            deathPending = true;
            shakeOnDeath = false;
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

    struct MovingSupport
    {
        MovingCube* cube;
        bool wasVerticallySupported;
    };
    std::vector<MovingSupport> movingSupports;
    for (const auto& cube:cubes)
    {
        auto* movingCube = dynamic_cast<MovingCube*>(cube.get());
        if (!movingCube)
            continue;

        const sf::Vector3f cubePosition = movingCube->getPosition();
        const sf::Vector3f cubeSize = movingCube->getSize();
        const sf::Vector3f playerPosition = player.getPosition();
        const float platformTop = cubePosition.z + cubeSize.z;
        const bool isVerticallySupported =
            playerPosition.z >= platformTop - 0.5f
            && playerPosition.z <= platformTop + 0.5f
            && player.collidesWith(*movingCube);

        movingSupports.push_back({movingCube,isVerticallySupported});
    }

    for (const auto& cube:cubes)
        cube->update(dt);
    for (const auto& spike:spikes)
        spike->update(dt);

    for (const MovingSupport& support:movingSupports)
    {
        const sf::Vector3f movement = support.cube->getMovementDelta();
        if (support.wasVerticallySupported
            && std::abs(movement.z) > 0.001f)
            player.setPosition(player.getPosition()
                + sf::Vector3f(0.f,0.f,movement.z));

        const bool hasHorizontalMovement =
            std::abs(movement.x) > 0.001f
            || std::abs(movement.y) > 0.001f;
        if (!hasHorizontalMovement)
            continue;

        const sf::Vector3f cubePosition = support.cube->getPosition();
        const sf::Vector3f cubeSize = support.cube->getSize();
        const sf::Vector3f playerPosition = player.getPosition();
        const sf::Vector2f playerSize = player.getSize();
        const float cubeBottom = cubePosition.z;
        const float cubeTop = cubePosition.z + cubeSize.z;
        const float playerBottom = playerPosition.z;
        const float playerTop = playerPosition.z + playerSize.y;
        const bool hasStrictVerticalOverlap =
            playerTop > cubeBottom && playerBottom < cubeTop;
        if (!hasStrictVerticalOverlap)
            continue;

        const sf::Vector2f cubeCenter(
            cubePosition.x + cubeSize.x * 0.5f,
            cubePosition.y + cubeSize.y * 0.5f
        );
        const sf::Vector2f playerOffset(
            playerPosition.x - cubeCenter.x,
            playerPosition.y - cubeCenter.y
        );
        const float approach =
            movement.x * playerOffset.x + movement.y * playerOffset.y;

        if (approach > 0.f && player.collidesWith(*support.cube))
            player.setPosition(
                player.getPosition()
                + sf::Vector3f(movement.x,movement.y,0.f)
            );
    }

    player.update(dt,cubes);

    for (const auto& spike:spikes)
    {
        if (player.collidesWith(*spike))
        {
            deathPending = true;
            shakeOnDeath = true;
            deathElapsed = 0.f;
            return;
        }
    }

    if (!restartRequested && player.getPosition().z < -500.f)
    {
        deathPending = true;
        shakeOnDeath = false;
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
        shakeOnDeath = false;
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
    sf::Vector2f shakeOffset(0.f,0.f);
    if (shakeOnDeath && deathPending)
    {
        constexpr float shakeDuration = 1.f;
        constexpr float shakeAmplitude = 14.f;
        const float progress = std::clamp(deathElapsed / shakeDuration,0.f,1.f);
        const float amplitude = shakeAmplitude * (1.f - progress);
        shakeOffset = {
            std::sin(deathElapsed * 71.f) * amplitude,
            std::cos(deathElapsed * 83.f) * amplitude
        };
    }

    const sf::Vector3f playerPosition = player.getPosition();
    const float playerAngle = player.getAngle();
    const sf::Vector2f forward = forwardAxis(playerAngle);
    worldView.setCenter({
        forwardPosition(cameraPosition,forward) - shakeOffset.x,
        -cameraPosition.z - shakeOffset.y
    });
    Display::setView(window,worldView);

    for (const auto& cube:cubes)
    {
        renderCubeSlice(window,*cube,playerPosition,playerAngle,sf::Color(70,110,180));
    }
    for (const auto& spike:spikes)
    {
        renderCubeSlice(window,*spike,playerPosition,playerAngle,sf::Color(220,50,50));
    }

    renderCubeSlice(window,*finishCube,playerPosition,playerAngle,sf::Color(255,210,40));

    const sf::Vector2f playerSize = player.getSize();
    sf::RectangleShape playerShape({playerSize.x,playerSize.y});
    playerShape.setPosition({
        forwardPosition(playerPosition,forward),
        -playerPosition.z - playerSize.y
    });
    playerShape.setFillColor(sf::Color(240,70,70));
    window.draw(playerShape);

    Display::setView(window,Display::virtualCenter - shakeOffset);

    timerText.setString("TIME  " + formatTime(levelTime));
    timerPanel.setPosition({24.f,24.f});
    timerText.setPosition({42.f,38.f});
    window.draw(timerPanel);
    window.draw(timerText);

    angleText.setString("ANGLE  " + formatAngle(playerAngle) + " deg");
    anglePanel.setPosition({230.f,24.f});
    window.draw(anglePanel);
    angleIndicator.setPosition({325.f,61.f});
    angleIndicator.setRotation(sf::degrees(-playerAngle * 180.f / 3.14159265358979323846f));
    window.draw(angleIndicator);
    angleText.setPosition({246.f,91.f});
    window.draw(angleText);

    Display::setView(window,Display::virtualCenter);

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