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
    std::unique_ptr<FinishCube> finishCube;
    Player player;
    sf::Vector3f cameraPosition;
    float cameraAngle;
    bool cameraInitialized;
    int levelIndex;
    bool instructionsShown;

public:
    LevelState(AssetManager& assets,std::queue<StateRequest>& stateRequests,int levelIndex);
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) override;
    void update(float dt,sf::RenderWindow& window) override;
    void render(sf::RenderWindow& window) override;
};