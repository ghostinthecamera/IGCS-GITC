Injectable Camera for Dark Souls 3
============================
![Dark Souls III 2025 04 21 - 23 29 50 01_2](https://github.com/user-attachments/assets/8e43a39c-eba0-425f-8cfe-b01c4c2dc038)  
Current supported game version: 1.15 and 1.15.2  
Camera version: 1.0.0  
Credits: ghostinthecamera

Changelog
========================
v1.0.0 - Initial Release

Features
===============
- Freecamera control
- Timestop/Gamespeed control
- Field of View control
- HUD Toggle
- Vignette/Post Process removal
- Player/NPC visibility 
- PLayersOnly (freeze all NPC characters)
- Path Controller
- Path Visualisation (via DX11 Hook which can be disabled)
- IGCSConnector support - IGCSDOF and Reshade States

Pause/Gamespeed Control
========================
The tools have full control over the gamespeed. You can choose an alternate timestop mode as the default approach can sometimes cause some environment geometry to disappear (hanging cages/bodies). In this alternate mode, animations on cloth do not stop completely.

You can set a slow motion factor to reduce the game speed (to help with that perfect shot) and this can be used with the game pause i.e. you can enable slow motion and then use the game pause hotkey to pause the game.

Player Only 
=======================
Enabling player only will freeze all enemies apart from the player, letting you do whatever you want :D

Path Controller
========================
Pretty self explanatory - Once the camera is active, you can save the position of the camera to create a path which you can then play. Useful for video etc.

Advanced settings has some additional settings - you can choose the type of interpolation for your path. 

Camera control device
========================
In the configuration tab of the IGCS Client, you can specify what to use for controlling the camera: controller, keyboard+mouse, or both. The device you pick is blocked from giving input to the game, if you press 'Numpad .' (On by default). 

About hotsampling support
==========================
No hotsampling supported
