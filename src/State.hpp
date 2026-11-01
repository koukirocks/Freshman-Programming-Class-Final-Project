#pragma once
#include "AssetManager.hpp"

#include <SFML/Graphics.hpp>
#include <vector>
#include <queue>
#include <memory>

enum class StateID {
    Null,
    Menu,
    LevelSelect,
    Level,
    LevelComplete,
    Pause,
    Instruct
};

struct StateRequest {

    enum class Type {
        Pop,
        Push
    };
    StateRequest(Type _type,StateID _stateID,int _levelIndex = -1):
    type(_type),
    stateID(_stateID),
    levelIndex(_levelIndex)
    {}

    Type type;
    StateID stateID;
    int levelIndex;
};

class State
{
protected:
    std::queue<StateRequest> &stateRequests;
    AssetManager &assets;

public:
    State(AssetManager &_assets, std::queue<StateRequest> &_stateRequests);
    virtual ~State() = default;

    virtual void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window) = 0;
    virtual void update(float dt,sf::RenderWindow& window) = 0;
    virtual void render(sf::RenderWindow& window) = 0;
};

class StateStack
{
private:
    std::vector<std::unique_ptr<State>> states;
    AssetManager &assets;

public:
    StateStack(AssetManager &_asstes);
    std::queue<StateRequest> stateRequests;
    void push(std::unique_ptr<State> state);
    void pop();
    State* top();
    void processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window);
    void update(float dt,sf::RenderWindow& window);
    void render(sf::RenderWindow& window);
    std::unique_ptr<State> createState(StateID stataID,int levelIndex = -1);
};