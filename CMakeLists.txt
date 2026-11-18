cmake_minimum_required(VERSION 3.20)

project(MyGame
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

add_executable(MyGame
    src/main.cpp
    src/Game.cpp
    src/Player.cpp
    src/AssetManager.cpp
    src/State.cpp
    src/states/MenuState.cpp
    src/states/LevelSelectState.cpp
    src/objects/Button.cpp
    src/game/GameObject.cpp
    src/game/Cube.cpp
    src/game/ObstacleCube.cpp
    src/game/MovingCube.cpp
    src/game/SpikeCube.cpp
    src/game/FinishCube.cpp
    src/game/Level.cpp
    src/game/Player.cpp
    src/states/LevelState.cpp
    src/states/PauseState.cpp
    src/states/LevelCompleteState.cpp
    src/states/InstructState.cpp
)

target_include_directories(MyGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
)

target_link_libraries(MyGame PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

add_custom_command(
    TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        "$<TARGET_FILE_DIR:MyGame>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:MyGame>/assets"
)

if(WIN32 AND TARGET OpenAL::OpenAL)
    add_custom_command(
        TARGET MyGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:OpenAL::OpenAL>"
            "$<TARGET_FILE_DIR:MyGame>"
        VERBATIM
    )
endif()