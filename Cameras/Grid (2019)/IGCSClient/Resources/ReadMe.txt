Injectable Camera for Grid 2019
============================
Current supported game version: Latest
Camera version: 1.0.2  
Credits: ghostinthecamera

Changelog
========================
v1.0.2 - Fixed bug where command queue capture would not capture a directx12 command list if it was not the first command list used in a frame.  
		 Code now captures direct command lists only.
v1.0.1 - Reshade connection bug fix which always returned true for reshade connected
v1.0.0 - Initial Release

Features
===============
- Freecamera control
- Timestop/Gamespeed control
- Field of View control
- HUD Toggle
- Depth of Field Removal
- In Game motion blur control
- Path Controller
- Path Visualisation
- Player Look At Visualisation (via DX12 Hook which can be disabled)
- Multiple freecamera and path controller camera modes (look at player, track player speed)
- Camera shake (free cam and path controller)
- IGCSConnector support - IGCSDOF and Reshade States
- Customisable Keys/Gamepad Buttons
- Shadow resolution increase


Free Camera
=======================
The game uses different cameras for different views. So the camera view that you are in when you enable the free camera with INS can have significant effects on what is rendered. I.e. if you enable while you are in interior cam, then the exterior of the car is not rendered at all. The best view to use is the chase camera views.

There are three modes:
- Normal free camera
- Look at player (enable through configuration page)
In this mode the camera will always look at the player object. You can move the camera as normal.
In this mode there are two ways of adjusting the look at point: Target offset or Angle offset
Target Offset: In this mode, the right stick will offset the look at point relative to the car position. i.e. moving the right stick up will move the offset point forward out of the cars front. Use LB+LT/RT to move the point up or down. Use LB+RB+R3 or Shift+F9 to display a sphere at the coordinates to help visualise. This is only available if the D3DHook is enabled.
Angle Offset: In this mode the right stick will offset the look at point by an angle, or another way to think about it is in screenspace. I.e. moving the right stick up will ensure that the camera is always rotated up from that point by that amount. There is only right and left possibilities here.
- Camera Mount (enable by present LB+RB+A or Shift+F8 in normal free camera or look at player mode.
In this mode, when the hot key is pressed, the camera will be locked in its position to the rotations and position of the player object. I.e. if the camera was mounted onto the car with a stick. Useful for generated motion blur with reshade.

Camera shake can be used with all these modes.

Car Tracking
=======================
The default tracking mode for the look at/camer mount camera works best in replay mode.
If, for some reason, you want to use the look at or the camera mount during gameplay, the alternate tracking mode is better.

Path Visualisations
=======================
The tools hook into DirectX to be able to provide a visualisation overlay. By its nature it can be problematic, if you are experiencing issues, you can disable D3D hooking prior to injection. This will only impact visualisations.

The tools also use the game's depth buffer to provider a more pleasant experience with visualisations. You will need to:
1) Enable depth buffer usage (F9)
2) Cycle through depth buffers and select the correct one using the Tab key. 

This is due to the way DX12 works, unfortunately. If you hotsample or resize the window depth buffer usage will be disabled and you need to repeat steps 1-2.

Pause/Gamespeed Control
========================
The tools have full control over the gamespeed. The game is paused by setting the speed to 0. You can set the speed to any value between 0 and 2. 

You can set a slow motion factor to reduce the game speed (to help with that perfect shot) and this can be used with the game pause i.e. you can enable slow motion and then use the game pause hotkey to pause the game. However, if you try and use this during gameplay (i.e. non-replay) and set a gamespeed between 0.01 and 0.99 the game will stutter. 

That said, I recommend using the game's built in game speed controls during replay. You can use the tools game speed as a multiplier (i.e. if you set the replay to 1/2 and then the tools are set to 0.5 you will effectively get 0.5*0.5 = 0.25 as the game speed.

HUD Toggle
=======================
You can toggle the HUD using the tools. It doesn't get rid of everything. If you are using replay mode, the in game toggle is better and recommended

Path Controller
========================
Pretty self explanatory - Once the camera is active, you can save the position of the camera to create a path which you can then play. Useful for video etc.

Advanced settings has some additional settings:
- Path visualisation (only available if D3DHook enabled)
- Play after delay
- Unpause game on play (only works when you use the tools gamespeed settings)
- Play path relative to player
- Look at player during path (this ignores whatever rotation is saved with each node)
- Adaptive path speed (the path speed is adjusted by the speed of the player)
- Interpolation modes 
- Easing modes
- Sample size for the path interpolation. Higher values use larger samples and could create smoother curves.
- Option to use fixed delta rather than a calculated one based on real frametimes

Camera control device
========================
In the configuration tab of the IGCS Client, you can specify what to use for controlling the camera: controller, keyboard+mouse, or both. The device you pick is blocked from giving input to the game, if you press 'Numpad .' (On by default). 

Shadow Resolution Increase
========================
Enabling this changes the shadow resolution from 2048 to 4096. During a resize event (in game/hotsampling) this is reverted, because if left enabled it breaks the game shadows post resize (it's never straightforward). You will need to uncheck and recheck this to bring it back after a resize event.

About hotsampling support
==========================
The game does support hotsampling. Using hotsampling or resizing the window while any visualisation is enabled has resulted in sporadic crashes so i would recommend disabling all visualisation before resizing (either through in game or hotsampling)

Default Keybindings
====================
The following is a list of the default keybindings. You can change these in the 'Keybindings' tab in the client.

Keyboard & Mouse
----------------

CAMERA
  Enable/Disable Camera: Insert - Toggles the custom camera on and off.
  Lock/Unlock Camera Movement: Home - Locks and unlocks the camera's position and orientation.
  Block Input to Game: Numpad . (Decimal) - Blocks keyboard and mouse input to the game.

MOVEMENT
  Move Forward: Numpad 8 - Moves the camera forward.
  Move Backward: Numpad 5 - Moves the camera backward.
  Move Left: Numpad 4 - Moves the camera to the left.
  Move Right: Numpad 6 - Moves the camera to the right.
  Move Up: Numpad 9 - Moves the camera up.
  Move Down: Numpad 7 - Moves the camera down.

ROTATION
  Rotate Up: Up Arrow - Rotates the camera upwards (pitch).
  Rotate Down: Down Arrow - Rotates the camera downwards (pitch).
  Rotate Left: Left Arrow - Rotates the camera to the left (yaw).
  Rotate Right: Right Arrow - Rotates the camera to the right (yaw).
  Tilt Left: Numpad 1 - Tilts (rolls) the camera to the left.
  Tilt Right: Numpad 3 - Tilts (rolls) the camera to the right.

FIELD OF VIEW (FoV)
  Increase FoV: Numpad + (Add) - Increases the camera's field of view.
  Decrease FoV: Numpad - (Subtract) - Decreases the camera's field of view.
  Reset FoV: Numpad * (Multiply) - Resets the field of view to its default value.

GAME CONTROL
  Pause/Unpause Game: Numpad 0 - Pauses or unpauses the game.
  Skip Frame(s): Page Down - Advances the game by a single frame when paused.
  Slow Motion: Page Up - Toggles slow motion effect.
  Toggle HUD: Delete - Toggles the game's Heads-Up Display on and off.

DEPTH OF FIELD / RESHADE
  Cycle Depth Buffers: Tab - Cycles through available depth buffers for Reshade.
  Toggle Depth Buffer Usage: Tab - Toggles the depth buffer used by the camera tool.

LOOK AT FEATURES
  Toggle LookAt Visualization: F9 - Toggles visualization of the LookAt point.
  Reset LookAt Offsets: F5 - Resets the LookAt offsets to zero.
  Toggle Offset Mode: F6 - Toggles between camera-relative and world-relative offset modes.
  Toggle Height-Locked Movement: F7 - Toggles whether camera movement is locked to the current height.
  Toggle Fixed Camera Mount: F8 - Toggles a fixed camera mount point.
  Move Target Up: U - Moves the LookAt target up.
  Move Target Down: O - Moves the LookAt target down.

PATH CONTROLLER
  Create Path: F2 - Creates a new camera path.
  Add Node to Path: F4 - Adds the current camera position/orientation as a node to the path.
  Play/Stop Path: F3 - Starts or stops playing the current camera path.
  Cycle Path Visualization: F12 - Cycles through different path visualization modes.
  Delete Current Path: F10 - Deletes the currently selected path.
  Cycle to Next Path: ] - Switches to the next available camera path.
  Delete Last Node: F11 - Removes the last added node from the current path.


Gamepad
-------

MODIFIERS
  Fast Movement / Rotation: Y - Increases camera movement and rotation speed when held.
  Slow Movement / Rotation: X - Decreases camera movement and rotation speed when held.
  Path Controller Modifier: Left Shoulder - Hold this button to access Path Controller actions.

CAMERA
  Tilt Left: D-Pad Left - Tilts (rolls) the camera to the left.
  Tilt Right: D-Pad Right - Tilts (rolls) the camera to the right.

FIELD OF VIEW (FoV)
  Increase FoV: D-Pad Down - Increases the camera's field of view.
  Decrease FoV: D-Pad Up - Decreases the camera's field of view.
  Reset FoV: B - Resets the field of view to its default value.

LOOK AT FEATURES
  Reset LookAt Offsets: LB + B - Resets the LookAt offsets to zero.
  Toggle Offset Mode: LB + Y - Toggles between camera-relative and world-relative offset modes.
  Toggle Height-Locked Movement: LB + X - Toggles whether camera movement is locked to the current height.
  Toggle Fixed Camera Mount: LB + A - Toggles a fixed camera mount point.
  Toggle LookAt Visualization: LB + Right Stick - Toggles visualization of the LookAt point.

PATH CONTROLLER
  Create Path: LB + Y - Creates a new camera path.
  Add Node to Path: LB + A - Adds the current camera position/orientation as a node to the path.
  Play/Stop Path: LB + X - Starts or stops playing the current camera path.
  Cycle Path Visualization: LB + Left Stick - Cycles through different path visualization modes.
  Delete Current Path: LB + B - Deletes the currently selected path.
  Cycle to Next Path: LB + D-Pad Right - Switches to the next available camera path.
  Delete Last Node: LB + D-Pad Left - Removes the last added node from the current path.


