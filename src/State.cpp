#include "State.hpp"
#include "states/MenuState.hpp"
#include "states/LevelSelectState.hpp"
#include "states/LevelState.hpp"
#include "states/LevelCompleteState.hpp"
#include "states/PauseState.hpp"
#include "states/InstructState.hpp"
#include <algorithm>

#include <stdexcept>


State::State(AssetManager &_assets, std::queue<StateRequest> &_stateRequests):
assets(_assets),
stateRequests(_stateRequests)
{}

StateStack::StateStack(AssetManager &_assets):assets(_assets) {
    ;
}

void StateStack::push(std::unique_ptr<State> state)
{
    states.push_back(std::move(state));
}

void StateStack::pop()
{
    if (states.empty()) return;
    states.pop_back();
}

State* StateStack::top()
{
    if (states.empty()) throw std::out_of_range("No state in statestack");
    return states.back().get();
}

void StateStack::processEvent(const std::optional<sf::Event>& event, sf::RenderWindow& window)
{
    top()->processEvent(event,window);
}

void StateStack::update(float dt,sf::RenderWindow& window)
{
    top()->update(dt,window);
    while (!stateRequests.empty()) {
        StateRequest request = stateRequests.front();
        stateRequests.pop();

        switch(request.type)
        {
            case StateRequest::Type::Pop: {
                pop();
                break;
            }
            case StateRequest::Type::Push: {
                push(createState(request.stateID,request.levelIndex));
                break;
            }
        }
    }
}

std::unique_ptr<State> StateStack::createState(StateID stateID,int levelIndex) {
    switch(stateID)
    {
        case StateID::Menu: {
            return std::make_unique<MenuState>(assets, stateRequests);
        }
        case StateID::LevelSelect: {
            return std::make_unique<LevelSelectState>(assets,stateRequests);
        }
        case StateID::Level: {
            return std::make_unique<LevelState>(assets,stateRequests,levelIndex);
        }
        case StateID::LevelComplete: {
            return std::make_unique<LevelCompleteState>(assets,stateRequests);
        }
        case StateID::Pause: {
            return std::make_unique<PauseState>(assets,stateRequests);
        }
        case StateID::Instruct: {
            return std::make_unique<InstructState>(assets,stateRequests,levelIndex);
        }
    }
    return nullptr;
}

void StateStack::render(sf::RenderWindow& window)
{
    for (auto &state:states) {
        state.get()->render(window);
    }
}