# 2-3D Platformer: Complete Project Breakdown and Report Guide

## 1. Executive summary

This project is a C++17 game built with CMake and SFML 3.1. It is a platform game with an unusual visual and gameplay idea: the level is stored as a three-dimensional arrangement of rectangular boxes, but the player sees a two-dimensional vertical slice through that world. The player can rotate the slice with **Q** and **E**. A route that is hidden from the current slice can become visible after rotating.

The program is organized around:

1. A fixed-resolution SFML window and game loop.
2. A stack of game states for menus, levels, overlays, and completion screens.
3. A data-driven level format in `assets/data/level`.
4. A player physics system that works in 3D world coordinates.
5. A slice renderer that converts the relevant part of each 3D cube into a 2D polygon.
6. A lightweight in-memory progress system for best stars and best times.

The project is therefore not a conventional 3D renderer. It is better described as a **2.5D or slice-based 3D platformer**: gameplay uses three dimensions, while the display shows a carefully chosen two-dimensional cross-section.

## 2. How to describe the game in a presentation

A concise description suitable for a report introduction is:

> This project is a data-driven C++ platform game made with SFML. It represents platforms, hazards, and goals as three-dimensional boxes, but renders the world as a vertical slice aligned with the player’s current angle. The player moves, jumps, rotates the viewing slice, avoids hazards, reaches the finish, and earns stars based on completion time.

The central design problem is:

> How can a three-dimensional level be made playable and understandable when the player is only shown one two-dimensional cross-section at a time?

The solution is to keep the simulation and collision detection in full 3D, but render each cube’s intersection with the player’s vertical slice. This gives the player a 2D platforming interface while preserving 3D navigation and rotation puzzles.

## 3. Technology and build configuration

The build is defined in [`CMakeLists.txt`](CMakeLists.txt).

### Technologies

- Language: C++17.
- Build system: CMake 3.20 or newer.
- Graphics/window/input library: SFML 3.1.0.
- Target: an executable named `MyGame`.
- Platform-specific build support: the current generated build is Visual Studio/MSBuild on Windows.

SFML is downloaded by CMake using `FetchContent`:

```cmake
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
)
```

The program links to:

- `SFML::Graphics`
- `SFML::Window`
- `SFML::System`

The CMake file also copies the whole `assets` directory next to the executable after building. This is important because the program loads files using paths such as `assets/fonts/NotoSansSC.ttf` and `assets/data/level/level1.level`.

### Build commands

From the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

The executable is normally produced at:

```text
build/Debug/MyGame.exe
```

The existing Debug build was verified successfully during preparation of this report.

## 4. Repository structure

```text
.
├── CMakeLists.txt
├── assets
│   ├── data/level
│   │   ├── level1.level
│   │   ├── level2.level
│   │   ├── level3.level
│   │   ├── level4.level
│   │   ├── level5.level
│   │   └── level6.level
│   ├── fonts
│   │   ├── NotoSansSC.ttf
│   │   └── arial.ttf
│   └── images
│       ├── menu_background.png
│       ├── sign.png
│       ├── star_empty.png
│       └── star_filled.png
├── src
│   ├── main.cpp
│   ├── Game.cpp / Game.hpp
│   ├── State.cpp / State.hpp
│   ├── AssetManager.cpp / AssetManager.hpp
│   ├── Display.hpp
│   ├── LevelProgress.hpp
│   ├── UITheme.hpp
│   ├── game
│   ├── objects
│   └── states
└── build
```

The `build` directory is generated build output. The important source and content directories are `src` and `assets`.

## 5. Program startup and the main loop

### 5.1 Entry point

[`src/main.cpp`](src/main.cpp) is intentionally small:

1. Construct a `Game`.
2. Call `game.run()`.
3. Catch startup/runtime exceptions and print an error.
4. Return a non-zero exit code if startup fails.

This keeps application startup separate from the game implementation.

### 5.2 Game ownership

[`src/Game.hpp`](src/Game.hpp) defines the main owner object. It owns:

- `sf::RenderWindow window`
- `sf::View gameView`
- `AssetManager assets`
- `StateStack stateStack`

The constructor in [`src/Game.cpp`](src/Game.cpp):

1. Creates a 1280×720 window.
2. Gives it the title `My Game`.
3. Allows title-bar, close, and resize operations.
4. Creates a view using the virtual resolution.
5. Limits rendering to 60 frames per second.
6. Configures the viewport.
7. Pushes the initial `Menu` state.

### 5.3 Frame-by-frame order

`Game::run()` executes while the window is open. Each frame follows this order:

```text
restart clock and calculate dt
        ↓
poll all window events
        ↓
handle resize and forward events to the top state
        ↓
update the top state
        ↓
apply queued state push/pop requests
        ↓
clear the window
        ↓
render every state in the stack
        ↓
display the finished frame
```

`dt` means delta time: the number of seconds since the previous frame. Movement and timers multiply their speeds by `dt`, making behavior less dependent on the frame rate.

### 5.4 Display scaling

[`src/Display.hpp`](src/Display.hpp) defines a virtual resolution of 1280×720. The real window may be resized, but the game keeps this logical coordinate system.

`Display::updateViewport()` compares the real window aspect ratio with the virtual aspect ratio:

- If the window is wider, it adds horizontal letterboxing.
- If the window is taller, it adds vertical letterboxing.
- If the ratios match, the viewport fills the window.

`Display::mousePosition()` converts physical mouse coordinates into the virtual game coordinates. This is what makes button hit testing continue to work after resizing.

## 6. The state machine

### 6.1 Why a state stack is used

The game has several distinct screens and overlays. Instead of placing every possible mode in one large class, it uses polymorphic states.

[`src/State.hpp`](src/State.hpp) defines:

```cpp
enum class StateID {
    Null,
    Menu,
    LevelSelect,
    Level,
    LevelComplete,
    Instruct,
    Pause
};
```

Each state implements three operations:

- `processEvent()`: reacts to SFML events.
- `update()`: changes gameplay/UI over time.
- `render()`: draws the state.

### 6.2 State requests

States do not directly modify the vector owned by `StateStack`. Instead, they place a `StateRequest` in a queue.

A request contains:

- `Type`: `Push` or `Pop`.
- `StateID`.
- Optional level index.
- Optional completion time.
- Optional star count.

This delayed approach prevents a state from destroying or replacing itself in the middle of event processing.

After the top state updates, `StateStack::update()` consumes the queue:

- `Pop` removes the top state.
- `Push` constructs a new state through `createState()`.

The stack sends input and updates only to the top state. It renders all states from bottom to top. That rendering rule makes overlays possible:

```text
Level
└── Pause overlay
```

The level remains below the pause screen, so the pause screen can draw a dark background over it.

### 6.3 State flow

The normal flow is:

```text
Menu
  ↓ Play
LevelSelect
  ↓ choose a level
Level
  ↓ first update
Instruct overlay
  ↓ finish instructions
Level
  ↓ touch finish
LevelComplete
  ├── Next Level → Level
  ├── Retry      → Level
  ├── Level Select
  └── Main Menu
```

Pressing **Escape** during a level pushes `Pause`. Pause supports:

- Resume: pop the pause state.
- Level Select: remove pause and level, then push level select.
- Main Menu: remove pause and level, then push menu.

### 6.4 State responsibilities

#### `MenuState`

Implemented in [`src/states/MenuState.cpp`](src/states/MenuState.cpp).

- Displays the menu background.
- Displays the title `2-3D PLATFORMER`.
- Displays the Play button.
- Updates button hover state.
- Changes to `LevelSelectState` when Play is clicked.

#### `LevelSelectState`

Implemented in [`src/states/LevelSelectState.cpp`](src/states/LevelSelectState.cpp).

- Finds all `.level` files.
- Creates one button per level.
- Displays three star slots per level.
- Displays a Main Menu button.
- Opens `LevelState` with the selected zero-based level index.

The button label is one-based for players (`1`, `2`, `3`, etc.), while the internal vector index is zero-based.

#### `InstructState`

Implemented in [`src/states/InstructState.cpp`](src/states/InstructState.cpp).

- Loads the selected level’s instruction entries.
- Shows one instruction at a time.
- Converts escaped text such as `\n` into actual line breaks during level loading.
- Advances on a button click.
- Closes when the final instruction is accepted.
- Can be dismissed with Escape.

#### `LevelState`

Implemented in [`src/states/LevelState.cpp`](src/states/LevelState.cpp).

This is the main gameplay state. It owns:

- Parsed `LevelData`.
- Solid cubes.
- Spike cubes.
- One finish cube.
- One player.
- Camera position and angle.
- Timer and star calculation.
- Death/restart animation state.
- HUD objects.

#### `PauseState`

Implemented in [`src/states/PauseState.cpp`](src/states/PauseState.cpp).

It draws a translucent black background, a panel, and buttons over the level.

#### `LevelCompleteState`

Implemented in [`src/states/LevelCompleteState.cpp`](src/states/LevelCompleteState.cpp).

- Displays completion time.
- Displays best time.
- Displays earned stars.
- Displays the three star standards from the level file.
- Provides retry/navigation buttons.

## 7. Asset management

[`src/AssetManager.hpp`](src/AssetManager.hpp) and [`src/AssetManager.cpp`](src/AssetManager.cpp) provide cached access to fonts and textures.

When a texture or font is requested:

1. Look for an existing object in the corresponding unordered map.
2. If present, return the existing reference.
3. Otherwise resolve the path.
4. Load the file.
5. Throw a `std::runtime_error` if loading fails.
6. Store the object in a `std::unique_ptr`.
7. Return a reference to the cached resource.

The path resolver first checks the current working directory, then tries one directory above it. This supports running from either the repository root or a build directory.

Textures are configured with smoothing disabled. This is appropriate for the game’s crisp, geometric visual style.

## 8. UI helpers and buttons

### 8.1 Theme

[`src/UITheme.hpp`](src/UITheme.hpp) centralizes:

- Background color.
- Panel color and border.
- Accent and hover colors.
- Main and muted text colors.
- Text construction.
- Text centering.
- Panel creation.

Centralizing these values prevents each state from inventing a different visual language.

### 8.2 Button

[`src/objects/Button.hpp`](src/objects/Button.hpp) and [`src/objects/Button.cpp`](src/objects/Button.cpp) implement reusable buttons.

A button stores either:

- `sf::RectangleShape`, or
- `sf::CircleShape`.

This is implemented with `std::variant`.

Important behavior:

- Rectangle hit testing uses global bounds.
- Circle hit testing transforms the mouse into the circle’s local coordinates and checks the radius equation.
- Hovering changes the fill color.
- Button text is centered automatically.
- `setSize()` reduces the text size when the label is too large for the button.

## 9. Object-oriented game model

### 9.1 `GameObject`

[`src/game/GameObject.hpp`](src/game/GameObject.hpp) is the abstract base class for world objects.

It defines:

- `processEvent()`
- `update()`
- `render()`
- `getPosition()`
- `setPosition()`

The position is an `sf::Vector3f`, so every object can exist in 3D world coordinates.

### 9.2 `Cube`

[`src/game/Cube.hpp`](src/game/Cube.hpp) extends `GameObject` with a 3D size:

```text
size.x = width
size.y = depth
size.z = height
```

It provides `getSize()` and `setSize()`.

### 9.3 Cube types

The cube subclasses are:

- `ObstacleCube`: solid collision geometry.
- `MovingCube`: solid geometry that travels along a configured vector.
- `SpikeCube`: lethal collision geometry.
- `FinishCube`: the goal area.

At present, most cube subclasses have empty event/update/render methods because `LevelState` performs the actual slice rendering centrally. Their classes still communicate intent and allow future behavior to be added without changing the level representation.

### 9.4 The active player

The active player is [`src/game/Player.hpp`](src/game/Player.hpp), not the separate legacy-looking root-level player in [`src/Player.hpp`](src/Player.hpp).

The gameplay player stores:

- 3D position inherited from `GameObject`.
- 2D displayed size.
- Slice angle.
- Grounded state.
- Vertical speed.
- Forward speed.
- Gravity.
- Jump strength.
- Movement speed.
- Rotation speed.

Initial constants are:

```text
gravity       = 1000
jumpStrength  = 600
moveSpeed     = 300
rotationSpeed = 2.5 radians/second
```

## 10. Level data format

Levels are plain text files in `assets/data/level`.

`getLevelFiles()`:

1. Looks in `assets/data/level`.
2. Keeps regular files ending in `.level`.
3. Sorts names such as `level1.level`, `level2.level`, and `level10.level` numerically instead of alphabetically.

### 10.1 Supported commands

```text
player_position x y z
player_size width height
star_times fastTime mediumTime slowTime
instruction "text" textX textY textSize "button" buttonX buttonY buttonWidth buttonHeight
cube x y z width depth height
moving_cube x y z width depth height travelX travelY travelZ speed
spike x y z width depth height
finish x y z width depth height
```

### 10.2 Meaning of each command

#### `player_position`

The player’s starting world coordinate:

```text
player_position -350 50 200
```

The three numbers are X, Y, and Z.

#### `player_size`

The player’s visible/collision dimensions:

```text
player_size 80 80
```

The player is represented as a rectangle in the viewing slice, with horizontal size 80 and vertical size 80.

#### `cube`

Adds a stationary solid box:

```text
cube x y z width depth height
```

#### `moving_cube`

Adds a solid box that moves from its starting point in the direction of `travel`:

```text
moving_cube x y z width depth height travelX travelY travelZ speed
```

The endpoint is:

```text
startPosition + travel
```

The platform moves forward to the endpoint and then backward to the start.

#### `spike`

Adds a hazard box. Any collision causes the player to enter the death/restart sequence.

#### `finish`

Defines the goal box. Every level must have exactly the required finish data for successful loading.

#### `instruction`

Adds an instruction page. Text is quoted and supports escaped `\n`, `\r`, `\t`, and `\\`.

#### `star_times`

Defines three increasing time thresholds. The thresholds are checked from fastest to slowest:

```text
time <= starTimes[0] → 3 stars
time <= starTimes[1] → 2 stars
time <= starTimes[2] → 1 star
otherwise            → 0 stars
```

### 10.3 Validation

The loader throws errors for:

- Missing files.
- Unknown entries.
- Invalid quoted strings.
- Missing player position.
- Missing player size.
- Missing finish.
- Missing instruction.
- Missing star thresholds.
- Star thresholds that are not ascending.

This is valuable because malformed level content fails explicitly instead of producing an invisible or partially initialized level.

## 11. Player physics in detail

The physics implementation is in [`src/game/Player.cpp`](src/game/Player.cpp).

### 11.1 Input

During gameplay:

- **A**: move backward along the current slice.
- **D**: move forward along the current slice.
- **W**: jump if grounded.
- **Q**: rotate the slice counterclockwise/forward according to the implementation.
- **E**: rotate in the opposite direction.
- **R**: restart the current level.
- **Escape**: open pause.

The player’s forward direction is:

```text
forward = (cos(angle), sin(angle))
```

Therefore forward motion changes world X and Y based on the current angle:

```text
moveX = cos(angle) * speed * dt
moveY = sin(angle) * speed * dt
```

### 11.2 Opposite movement keys

The code starts each update with zero forward speed:

```text
forwardSpeed = 0
```

Then A subtracts movement speed and D adds movement speed. If both are held, they cancel rather than one arbitrary key winning.

### 11.3 Jumping and gravity

Jumping is allowed only when `isOnGround` is true. A jump sets:

```text
upSpeed = jumpStrength
isOnGround = false
```

Every physics sub-step applies gravity:

```text
upSpeed -= gravity * stepDt
```

Downward speed is capped at `-1500` to prevent unbounded acceleration.

### 11.4 Frame-time safety

The physics step clamps one frame’s accepted time to `0.05` seconds. It then divides that time into sub-steps no larger than `1/240` seconds.

This prevents two common problems:

1. A paused or overloaded frame producing an enormous movement jump.
2. The player moving completely through a thin platform between collision checks.

### 11.5 Rotation collision protection

Before changing the angle, the code checks whether the new slice would overlap a cube. If it would, rotation is rejected.

This prevents the player from rotating directly into solid geometry and becoming embedded.

### 11.6 Horizontal collision and wall sliding

The code first tests the complete horizontal movement:

```text
candidate = position + (moveX, moveY, 0)
```

If that collides, it separately tests:

1. X-only movement.
2. Y-only movement.

This produces wall sliding: if one axis is blocked, the player may still move along the other axis.

### 11.7 Vertical collision

For each solid cube:

- If falling and the player crosses a cube top, the player lands on the highest valid top.
- If the player intersects a ceiling or side, vertical movement is blocked.
- Otherwise the candidate vertical position is accepted.

The code checks every cube rather than stopping at the first one. This matters when multiple platforms overlap at different heights.

## 12. The collision algorithm: the important idea

The game’s collision is not a simple 2D rectangle check.

The player is represented by a line/rectangle moving forward inside a vertical plane. A cube occupies a 3D axis-aligned box. The code determines whether the player’s ground line intersects the cube’s X/Y footprint using a slab-clipping method.

Conceptually:

1. The player’s current X/Y point is the origin of a line.
2. The line direction is `(cos(angle), sin(angle))`.
3. For each world axis, calculate where the line enters and exits the cube’s interval.
4. Combine those intervals into one forward-distance interval.
5. Check whether that interval overlaps the player’s horizontal extent.
6. Separately check vertical overlap in Z.

This gives a collision test consistent with the slice renderer. The same angle that determines what is drawn also determines what can collide.

The helper distinguishes between:

- Strict overlap: used for normal blocking.
- Touching-inclusive overlap: used for landing and finish detection.

That distinction prevents small numerical gaps from making a platform impossible to land on, while avoiding false collision when objects only touch in a non-supporting way.

## 13. Moving platforms

[`src/game/MovingCube.cpp`](src/game/MovingCube.cpp) stores:

- Start position.
- Previous position.
- Travel vector.
- Speed.
- Current distance along the travel path.
- Direction, either forward or backward.
- Total travel length.

Each update:

1. Save the previous position.
2. Add `direction * speed * dt` to the path distance.
3. Clamp at either endpoint and reverse direction.
4. Convert distance to a fraction.
5. Set:

```text
position = startPosition + travel * fraction
```

`getMovementDelta()` returns the difference between the current and previous position.

`LevelState` uses this delta to:

- Move the player vertically if standing on the moving platform.
- Carry the player horizontally when the platform pushes into them and they are overlapping vertically.

Without this support logic, the platform would move under the player and the player would appear to slip or be left behind.

## 14. Level update sequence

The central update sequence in `LevelState::update()` is:

1. If death is pending, advance the death timer and wait for restart.
2. Advance the fade-in timer.
3. Advance the level timer after instructions are dismissed.
4. Detect whether the player is supported by moving platforms.
5. Update all solid and spike objects.
6. Apply moving-platform displacement to the player.
7. Run player input, rotation, movement, jumping, gravity, and collision.
8. Test collision with every spike.
9. Test whether the player has fallen below the world.
10. Initialize and update the camera.
11. Push the instruction overlay on the first playable update.
12. Check collision with the finish.
13. Calculate stars and transition to the completion screen.

The ordering is deliberate. Platforms must move before player physics, and hazards must be checked after the player’s new position has been calculated.

## 15. Camera and slice rendering

### 15.1 Camera behavior

The camera stores a 3D world position and a slice angle.

At the start of a level:

- The camera starts based on the player’s position.
- The player is positioned near the center of the view.
- The camera angle matches the player angle.

The camera supports:

- Automatic follow when the player moves.
- Manual panning with arrow keys.
- Smooth follow using exponential responsiveness.
- Edge margins so the player does not immediately touch the edge of the screen.
- A lower camera bound during falling.

### 15.2 Computing a cube slice

Rendering is performed by helper functions in `LevelState.cpp`:

- `cubeSlice()`
- `renderCubeSlice()`

The renderer:

1. Builds the eight vertices of a 3D cube.
2. Defines the twelve cube edges.
3. Defines the player-facing slice plane using:
   - Forward vector: `(cos(angle), sin(angle))`.
   - Normal vector: `(-sin(angle), cos(angle))`.
4. Tests every cube edge against the plane.
5. Adds exact vertices and edge intersection points.
6. Projects each intersection into:
   - Horizontal screen distance along the forward direction.
   - Vertical distance from the player’s Z.
7. Sorts the points around their center.
8. Draws an SFML convex polygon.

The result is a polygon representing the cross-section of the cube with the viewing plane.

### 15.3 Why rendering is centralized

The cube subclasses have empty `render()` methods because the slice depends on the player’s current position and angle. `LevelState` has that information, so it renders all cube types itself and chooses colors:

- Solid platforms: blue.
- Spikes: red.
- Finish: gold.
- Player: red rectangle.

This keeps the projection logic in one place and ensures every object is rendered relative to the same camera and slice.

## 16. Death, restart, and completion

### 16.1 Death conditions

The player dies when:

- They collide with a spike.
- They fall below the configured world height.
- They press **R** to request a restart.

Spike death enables a screen shake. Falling and manual restart do not use the same shake behavior.

### 16.2 Death animation

Death uses a sequence:

1. Delay.
2. Fade toward black.
3. Hold black.
4. Pop the old level.
5. Push a fresh instance of the same level.

This rebuilds all level objects and resets position, timer, camera, moving platforms, and progress within the attempt.

### 16.3 Completion

When the player collides with the finish:

1. Calculate earned stars from the elapsed time.
2. Record the best star count.
3. Record the best time if it is faster.
4. Pop `LevelState`.
5. Push `LevelCompleteState` with:
   - Level index.
   - Completion time.
   - Earned stars.

## 17. Progress tracking

[`src/LevelProgress.hpp`](src/LevelProgress.hpp) contains inline maps:

```cpp
std::unordered_map<int, int> bestStars;
std::unordered_map<int, float> bestTimes;
```

Best stars use `std::max`, so replaying a level cannot reduce the stored star result.

Best times use a minimum comparison, so only faster times replace the previous record.

This progress is **not saved to disk**. It lasts only while the executable is running. Closing and reopening the game resets it.

## 18. What each current level teaches

The six included files are also a progression of mechanics.

### Level 1

Introduces:

- Basic left/right movement.
- The finish condition.

It is a straightforward route with a large starting platform and a gold finish.

### Level 2

Introduces:

- Jumping with W.
- Camera movement with arrow keys.
- Gaps and elevated platforms.

### Level 3

Introduces:

- The central 3D-slice mechanic.
- Rotating with Q/E.
- Finding paths that are separated in world space.
- Restarting with R.

It contains multiple instruction pages and several platforms arranged in different directions.

### Level 4

Expands the rotation puzzle:

- Several turns are required.
- A path is constructed from platforms at different world positions and heights.
- The angle indicator becomes important for orientation.

### Level 5

Tests sustained navigation:

- More complex connected geometry.
- Multiple heights.
- A larger set of platforms.
- A longer star-time window.

### Level 6

Introduces:

- Red spike hazards.
- Moving platforms.
- Gaps that require timing and platform use.

This level demonstrates the complete gameplay loop: movement, jump/navigation, hazard avoidance, moving-platform support, and finish detection.

## 19. Root-level `Player` versus gameplay `Player`

There are two classes named `Player`:

1. [`src/Player.hpp`](src/Player.hpp) and [`src/Player.cpp`](src/Player.cpp)
2. [`src/game/Player.hpp`](src/game/Player.hpp) and [`src/game/Player.cpp`](src/game/Player.cpp)

The root-level version is a simple earlier 2D rectangle that moves with W/A/S/D and displays its coordinates. It is not the player used by `LevelState`.

The active version is under `src/game` and implements the actual platformer mechanics. `LevelState.hpp` includes `game/Player.hpp`, so it uses the active version.

For a future cleanup, the unused root-level player could be removed or renamed, but it should not be confused with the gameplay implementation when explaining the project.

## 20. Strengths of the implementation

The project has several good engineering decisions:

- State responsibilities are separated.
- State transitions are queued instead of performed unsafely during iteration.
- Assets are cached and loading failures are explicit.
- Levels are data-driven rather than hard-coded into gameplay code.
- The level loader validates required fields.
- The virtual resolution keeps UI layout stable.
- Physics uses delta time.
- Physics clamps large frames and sub-steps collision.
- Rotation is collision-checked.
- Moving platforms expose their displacement so they can carry the player.
- Progress records best values instead of overwriting them with worse attempts.
- The renderer and collision system use the same slice angle.

## 21. Current limitations and honest discussion points

These are useful to mention in a technical reflection:

1. **Progress is session-only.** Best times and stars are not written to a save file.
2. **Rendering is centralized in `LevelState`.** Cube subclasses do not independently render themselves.
3. **Several object methods are placeholders.** `ObstacleCube`, `SpikeCube`, and `FinishCube` mainly provide type identity and future extension points.
4. **There are two player implementations.** The root-level one appears to be legacy code.
5. **The level format is custom and positional.** It is simple and readable, but not self-describing like JSON or a binary editor format.
6. **The game uses a single-threaded loop.** This is appropriate for the project size but does not parallelize loading or simulation.
7. **The game depends on valid asset working paths.** The resolver supports the repository/build layout, but packaging for arbitrary locations would need additional path handling.
8. **There is no automated gameplay test suite.** The build validates compilation, while gameplay behavior is mainly checked by running the executable.

These are not necessarily failures. They are scope decisions and natural areas for future development.

## 22. Suggested report structure for submission

If this document is being adapted into a formal school report, use this order:

1. **Introduction**
   - Game concept.
   - Motivation for the 2D slice of a 3D world.
2. **Objectives**
   - Build a playable platformer.
   - Support multiple levels.
   - Implement movement, jumping, rotation, hazards, and scoring.
3. **Tools and technologies**
   - C++17.
   - CMake.
   - SFML 3.1.
4. **System architecture**
   - Main loop.
   - State stack.
   - Asset manager.
   - Level loader.
5. **Gameplay implementation**
   - Player physics.
   - Collision algorithm.
   - Moving platforms.
   - Death and completion.
6. **Rendering**
   - Slice plane.
   - Cube intersection.
   - Camera and viewport.
7. **Level design**
   - Explain the six levels as a learning progression.
8. **User interface**
   - Menu.
   - Level selection.
   - Instructions.
   - Pause.
   - Completion.
9. **Testing**
   - Build verification.
   - Manual test cases.
10. **Limitations and future work**
    - Save data.
    - More hazards.
    - More visual assets.
    - Automated tests.
11. **Conclusion**
    - Summarize the result and the main technical achievement.

## 23. Manual test checklist

Use this checklist when demonstrating or writing the testing section:

- [ ] The program opens to the main menu.
- [ ] The menu background, title, and Play button appear.
- [ ] Clicking Play opens level selection.
- [ ] All six level buttons appear.
- [ ] Clicking a level opens the correct level.
- [ ] Instructions appear before the level can be played.
- [ ] Instruction pages advance correctly.
- [ ] Escape closes the instruction overlay.
- [ ] A and D move along the current slice.
- [ ] W jumps only while grounded.
- [ ] Q and E rotate the slice.
- [ ] The angle HUD changes while rotating.
- [ ] Arrow keys pan the camera.
- [ ] Solid cubes block movement.
- [ ] The player can slide along a wall.
- [ ] The player lands on platforms.
- [ ] Falling causes a restart sequence.
- [ ] Spikes cause death and screen shake.
- [ ] R restarts the current level.
- [ ] Moving platforms travel back and forth.
- [ ] The player is carried by a moving platform.
- [ ] Reaching the gold finish opens the completion screen.
- [ ] Completion time is shown.
- [ ] Stars correspond to the level’s time thresholds.
- [ ] Best stars do not decrease after a slower replay.
- [ ] Best times do not increase after a slower replay.
- [ ] Retry, next level, level select, and main menu buttons work.
- [ ] Resizing the window preserves the logical layout and mouse interaction.

## 24. Final conclusion

The project combines a conventional state-driven game architecture with an original rendering concept. The simulation treats the world as 3D boxes, but the player interacts through a controllable 2D slice. The most technically important parts are the custom level parser, the slice-based collision test, the cube cross-section renderer, and the state stack that coordinates menus, instructions, gameplay, pause, and completion.

The code is small enough to understand as a complete system, but it still demonstrates important game-programming concepts:

- Real-time loops.
- Delta-time movement.
- Input processing.
- Polymorphism.
- Resource caching.
- Custom file formats.
- Collision detection.
- Camera control.
- UI state transitions.
- Scoring and progression.

That combination is the main story of the project: a compact game engine structure supporting a distinctive gameplay mechanic.
