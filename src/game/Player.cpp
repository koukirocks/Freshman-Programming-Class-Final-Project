#include "game/Player.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <SFML/Graphics.hpp>

namespace
{
    // Safety limits for the physics step (tune to taste).
    constexpr float maxFrameTime = 0.05f;      // longest dt we accept in one frame
    constexpr float maxSubStepTime = 1.f / 240.f; // collision is tested at least this often
    constexpr float maxFallSpeed = 1500.f;     // terminal velocity

    // The world is 3D but the player lives in a vertical slice plane: it passes
    // through the player's position and runs along the forward direction.
    // What the renderer draws for a cube is the cross-section of that cube with
    // the plane. The cross-section of an upright box is a rectangle whose
    // forward extent is the part of the plane's ground line that lies inside
    // the cube's footprint, and whose height is the cube's full height.
    // We compute exactly that interval (slab clipping) so collision matches
    // what is on screen at every angle.
    //
    // horizontalTouchingCounts: touching along the forward axis counts as overlap.
    // verticalTouchingCounts:   touching along the vertical axis counts as overlap.
    // A plane lying exactly on a cube face always counts as cutting the cube,
    // because the renderer draws that face as a slice.
    bool overlaps(sf::Vector2f playerSize,float playerAngle,sf::Vector3f candidatePosition,const Cube& cube,
        bool horizontalTouchingCounts,bool verticalTouchingCounts)
    {
        const sf::Vector3f cubePosition = cube.getPosition();
        const sf::Vector3f cubeSize = cube.getSize();

        const float origin[2] = {candidatePosition.x,candidatePosition.y};
        const float direction[2] = {std::cos(playerAngle),std::sin(playerAngle)};
        const float minimum[2] = {cubePosition.x,cubePosition.y};
        const float maximum[2] = {cubePosition.x + cubeSize.x,cubePosition.y + cubeSize.y};

        // Forward-distance interval [sliceStart, sliceEnd] where the ground line
        // is inside the cube footprint, measured from candidatePosition.
        float sliceStart = -std::numeric_limits<float>::infinity();
        float sliceEnd = std::numeric_limits<float>::infinity();
        for (int axis = 0; axis < 2; ++axis)
        {
            if (std::abs(direction[axis]) < 1e-6f)
            {
                // Line runs parallel to this pair of faces.
                if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis])
                    return false;
                continue;
            }

            float first = (minimum[axis] - origin[axis]) / direction[axis];
            float second = (maximum[axis] - origin[axis]) / direction[axis];
            if (first > second)
                std::swap(first,second);
            sliceStart = std::max(sliceStart,first);
            sliceEnd = std::min(sliceEnd,second);
        }

        if (sliceStart > sliceEnd)
            return false;

        const bool forwardOverlaps = horizontalTouchingCounts
            ? sliceEnd >= 0 && sliceStart <= playerSize.x
            : sliceStart < sliceEnd && sliceEnd > 0 && sliceStart < playerSize.x;
        if (!forwardOverlaps)
            return false;

        const float playerBottom = candidatePosition.z;
        const float playerTop = candidatePosition.z + playerSize.y;
        const float cubeBottom = cubePosition.z;
        const float cubeTop = cubePosition.z + cubeSize.z;
        return verticalTouchingCounts
            ? playerTop >= cubeBottom && playerBottom <= cubeTop
            : playerTop > cubeBottom && playerBottom < cubeTop;
    }

    bool overlapsAny(sf::Vector2f playerSize,float playerAngle,sf::Vector3f candidatePosition,
        const std::vector<std::unique_ptr<Cube>>& cubes)
    {
        for (const auto& cube:cubes)
        {
            if (overlaps(playerSize,playerAngle,candidatePosition,*cube,false,false))
                return true;
        }
        return false;
    }
}

Player::Player(sf::Vector3f _position,sf::Vector2f _size):
size(_size),
angle(0.0),
isOnGround(true),
upSpeed(0),
forwardSpeed(0),
upAcceleration(0),
forwardAcceleration(0),
gravity(1000),
jumpStrength(600),
moveSpeed(300),
rotationSpeed(2.5)
{
    setPosition(_position);
}

void Player::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
{
    ;
}

void Player::update(float dt)
{
    ;
}

void Player::update(float dt,const std::vector<std::unique_ptr<Cube>>& cubes)
{
    // FIX 4: never simulate a huge frame in one go.
    dt = std::min(dt,maxFrameTime);
    if (dt <= 0)
        return;

    // FIX 7: opposite keys cancel out instead of D winning.
    forwardSpeed = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        forwardSpeed -= moveSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        forwardSpeed += moveSpeed;

    // FIX 2: rotation is collision-checked; revert if it would put us inside a cube.
    float newAngle = angle;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
        newAngle += rotationSpeed * dt;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
        newAngle -= rotationSpeed * dt;
    if (newAngle != angle && !overlapsAny(size,newAngle,position,cubes))
        angle = newAngle;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) && isOnGround)
    {
        upSpeed = jumpStrength;
        isOnGround = false;
    }

    // FIX 4: sub-step so thin cubes can't be skipped over.
    const int steps = std::max(1,static_cast<int>(std::ceil(dt / maxSubStepTime)));
    const float stepDt = dt / static_cast<float>(steps);

    for (int step = 0; step < steps; ++step)
    {
        upSpeed -= gravity * stepDt;
        upSpeed = std::max(upSpeed,-maxFallSpeed);

        // ---- Horizontal movement (with wall sliding, FIX 6) ----
        const float moveX = std::cos(angle) * forwardSpeed * stepDt;
        const float moveY = std::sin(angle) * forwardSpeed * stepDt;

        sf::Vector3f candidatePosition = position + sf::Vector3f(moveX,moveY,0);
        if (!overlapsAny(size,angle,candidatePosition,cubes))
        {
            position = candidatePosition;
        }
        else
        {
            candidatePosition = position + sf::Vector3f(moveX,0,0);
            if (moveX != 0 && !overlapsAny(size,angle,candidatePosition,cubes))
            {
                position = candidatePosition;
            }
            else
            {
                candidatePosition = position + sf::Vector3f(0,moveY,0);
                if (moveY != 0 && !overlapsAny(size,angle,candidatePosition,cubes))
                    position = candidatePosition;
            }
        }

        // ---- Vertical movement (FIX 9: consider every overlapping cube) ----
        candidatePosition = position + sf::Vector3f(0,0,upSpeed * stepDt);
        isOnGround = false;

        bool landing = false;
        bool blocked = false;
        float landingHeight = 0;
        for (const auto& cube:cubes)
        {
            const sf::Vector3f cubePosition = cube->getPosition();
            const sf::Vector3f cubeSize = cube->getSize();
            const float cubeTop = cubePosition.z + cubeSize.z;

            // Landing: horizontal touching counts as support. The player is an
            // infinitely thin line sideways, so standing exactly on a cube's
            // edge line must still count as standing on it.
            if (upSpeed <= 0 && position.z >= cubeTop
                && overlaps(size,angle,candidatePosition,*cube,true,false))
            {
                landingHeight = landing ? std::max(landingHeight,cubeTop) : cubeTop;
                landing = true;
            }
            // Ceiling / embedded: only a real (strict) overlap blocks.
            else if (overlaps(size,angle,candidatePosition,*cube,false,false))
            {
                blocked = true;
            }
        }

        if (landing)
        {
            position.z = landingHeight;
            upSpeed = 0;
            isOnGround = true;
        }
        else if (blocked)
        {
            // Hit a ceiling (or embedded in a side): stop vertical motion only.
            upSpeed = 0;
        }
        else
        {
            position = candidatePosition;
        }
    }
}

void Player::render(sf::RenderWindow& window) const
{
    ;
}

sf::Vector2f Player::getSize() const
{
    return size;
}

float Player::getAngle() const
{
    return angle;
}

bool Player::collidesWith(const Cube& cube) const
{
    return overlaps(size,angle,position,cube,true,true);
}