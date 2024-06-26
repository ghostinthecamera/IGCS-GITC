Injectable Camera for Armored Core 6
============================

Current supported game version: App Ver .60     
Camera version: 2.11   
Credits: ghostinthecamera (with some tips from Frans!)     

- IGCSDOF support
- Timestop/Gamespeed control
- Field of View
- HUD Toggle
- Unlocked range in photomode
- Vignette Removal (If you are using the [ACVI Raw Graphics](https://www.nexusmods.com/armoredcore6firesofrubicon/mods/121) mod to remove Vignette then this will not work, so disable it)
 
Changelog
==============
x2.11: Addition of alternative timestop to mitigate crash and update to timescale system  
v2.01: Minor updates to user input code and timescale system  
v2.00: Initial Release  

Offline Use Only 
==================
As these tools work by injecting into the game's memory and running its own code, this will be flagged by anti-cheat systems. It is STRONGLY recommended to only use these in offline mode.  If you do use these while online, then there is a risk of being flagged by anti-cheat systems and the appropriate consequences being taken against you. 

Use at your own risk.

Timestop/Gamespeed
==================
There are two timestop methods available. The default method is more stable for normal use BUT will cause the game
to crash if used during the super boost game mechanic. In the 'Configuration' tab of this client, there is an option
to use an alternative timestop.  
  
The alternative timestop does not create this crash. However, it comes with its own compromises. The game may randomly
fly your mech to the top of the map when using it and this may be mission breaking. Some particle effects will constantly
be moving or jittering, and the game may accept movement after the timestop is applied meaning the mech may move slightly.  
  
Up to you to determine what you need.
  
Special Effects/Motion Blur:
=============
Contains a ShaderToggler.ini file for removal of effects such as the motion blur that stays on the player and enemy mechs when the game is paused. ShaderToggler is a reshade addon and is really needed for this to work. You can disable motion blur to remove this, or if you use these tools in conjunction with the photomode you can use the relevant slider to remove the blur. Alternatively, you can use shader toggler.

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
