Injectable Camera for Sekiro
============================
Current supported game version: 1.06 
Camera version: 2.10   
Credits: ghostinthecamera, Skall, Jim2point0 

Changelog
========================
v2.10 - Added alternate timestop using game pause
v2.00 - Initial Release

Features
===============
- IGCSDOF support
- Freecamera control
- Timestop/Gamespeed control (see notes below)
- Field of View control
- HUD Toggle
- DoF/Bloom removal during cutscenes when camera is enabled
- Gameplay FOV Adjustment for ultrawide

Pause/Gamespeed Control
========================
The tools have full control over the gamespeed and offers two ways to pause the game. The method bound to Num 0 will set the timescale to 0.0001. This functions in cutscenes, gameplay and dialogue (and anywhere else) and paused everything (player character, NPCs, environment). The method bound to the Pause key will pause the game as if you were pressing the pause button. This only functions during gameplay and does not stop the environment from continuing to animate (you can stop this by using the Num 0 pause in conjunction with this).

Timescale Method: For 99% of usecases this will suffice and function as expected. However, in certain situations pausing the game will not __completely__ stop movement for items such as the grappling hook and rope and clothing items. The character model itself does not move, so this should not pose an issue for most situations. This is only particularly problematic if you are using IGCSDOF (or require the image to be perfectly still for whatever reason) as the image is moving, which means you can never maintain focus.

Game Pause: During gameplay, this will pause the game and address the items which continue to move and address the 0.1% situation of the timescale method. You could use this in place of the timescale pause during gameplay. However, this has no imapct during cutscenes and you have to use the timescale method.

You can set a slow motion factor to reduce the game speed (to help with that perfect shot) and this can be used with the game pause i.e. you can enable slow motion and then use the game pause hotkey to pause the game.

Player Only 
=======================
Enabling player only will freeze all enemies apart from the player, letting you do whatever you want :D

Shader Toggler
=======================
A shader toggler file is supplied to remove the motion blur/TAA blur that remains sometimes when pausing the game. Disabling motion blur in the game settings will also prevent this problem from occurring.

Ultrawide FOV Adjustment
=======================
It is possible to change the gameplay FOV by enabling the relevant checkbox on the configuration tab. The slider can then be used to set the field of view. This might jank up the FOV used in other parts of the game (cutscene etc)

FPS Unlock
=======================
Sets the FPS cap to 120fps - it can make the game unstable so use at your own risk.

Camera control device
========================
In the configuration tab of the IGCS Client, you can specify what to use for controlling the camera: controller, keyboard+mouse, or both. The device you pick is blocked from giving input to the game, if you press 'Numpad .' (On by default). 

About hotsampling support
==========================
No hotsampling supported
