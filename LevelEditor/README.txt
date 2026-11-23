Freshman Level Editor
=====================

Open index.html in a modern browser. This editor is intentionally outside the
game repository and has no dependencies or build step.

1. Click "Open .level" and choose a file from the game's assets\data\level
   directory.
2. Orbit with the left mouse button, pan with Shift+drag, and zoom with the
   mouse wheel.
3. Select platforms, the finish, or the player start in the scene list.
4. Edit transforms in the right panel. Add or delete platforms as needed.
5. Hide any object (platform, moving platform, spike, finish, or player start) to
   see through it while building: use the Hide button next to it in the scene
   list, the Hide button in the right panel, or press H with it selected.
   Hidden objects are not drawn and can't be clicked in the scene; select them
   from the list (they show as a dashed outline). "Show all hidden objects"
   brings everything back. Hiding is editor-only: it is never saved, so hidden
   objects are still written to the .level file.
6. Moving platforms are previewed in motion: each one travels from its start
   position by its Travel vector at Speed units per second, then back, forever.
   The purple dotted outline is the full space it sweeps through. Click "Stop moving platforms" to freeze them
   at their saved start positions while you edit, and "Play" to resume.
7. Click "Save .level", then copy the downloaded file back to
   C++ Game Project\assets\data\level.

The editor uses the game's existing player_position, player_size, cube, finish,
star_times, and instruction syntax. Its axes match the game: X and Y are the
horizontal ground plane, and Z is vertical.
