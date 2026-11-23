#pragma once

#include "State.hpp"
#include "game/Level.hpp"
#include "game/Player.hpp"
#include "game/FinishCube.hpp"

#include <memory>
#include <vector>

class LevelState : public State
{
private:
    LevelData level;
    std::vector<std::unique_ptr<Cube>> cubes;
    std::vector<std::unique_ptr<Cube>> spikes;
    std::unique_ptr<FinishCube> finishCube;
    Player player;
    sf::Vector3f cameraPosition;
    float cameraAngle;
    sf::View worldView;
    bool cameraInitialized;
    int levelIndex;
    bool instructionsShown;
    bool restartRequested;
    bool deathPending;
    bool shakeOnDeath;
    float deathElapsed;
    float fadeInElapsed;
    bool cameraFollowPending;
    float levelTime;
    int earnedStars;
    sf::RectangleShape timerPanel;
    sf::Text timerText;
    sf::RectangleShape anglePanel;
    sf::Text angleText;
    sf::ConvexShape angleIndicator;

public:
    LevelState(AssetManager& assets,std::queue<StateRequest>& stateRequests,int levelIndex);
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt,sf::RenderWindow& window) override;
    void render(sf::RenderWindow& window) override;
};