Injectable Camera for Armored Core 6
============================

Current supported game version: App Ver .60   
Camera version: 2.0   
Credits: ghostinthecamera (with some tips from Frans!)   

- IGCSDOF support
- Timestop/Gamespeed control
- Field of View
- HUD Toggle
- Unlocked range in photomode
- Vignette Removal (If you are using the [ACVI Raw Graphics](https://www.nexusmods.com/armoredcore6firesofrubicon/mods/121) mod to remove Vignette then this will not work, so disable it)
 
Special Effects:
=============
Contains a ShaderToggler.ini file for removal of effects such as the motion blur that stays on the player and enemy mechs when the game is paused. ShaderToggler is a reshade addon and is really needed for this to work.

The shader groupings are self explanatory - try them out and see what they do

Controller Use
==============
This game uses steaminput and not xinput. Therefore, you must make a choice between using a controller for playing the game or using a controller to control the IGCS camera.

If you choose the game:
- This is the default option
- You can use the keyboard or mouse to control the camera.
- The IGCS system will not process any gamepad input

If you choose to control the IGCS camera:
- Disable steaminput for the game in its properties in steam
- The game can only be played with keyboard and mouse
- The IGCS system will respond to gamepad input

Camera control device
========================
In the configuration tab of the IGCS Client, you can specify what to use for controlling the camera: 
controller, keyboard+mouse, or both. The device you pick is blocked from giving input to the game, 
if you press 'Numpad .' (On by default). 

About hotsampling support
==========================
No hotsampling support in this game
