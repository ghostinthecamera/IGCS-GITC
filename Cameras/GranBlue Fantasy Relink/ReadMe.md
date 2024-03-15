Injectable Camera for GranBlue Fantasy ReLink
============================

Current supported game version: 1.1.2 
Camera version: 2.01   
Credits: ghostinthecamera & Skall 

- IGCSDOF support
- Freecamera control
- Timestop/Gamespeed control (see notes below)
- Field of View control
- HUD Toggle
- DoF removal during cutscenes when camera is enabled
- Bloom removal when camera is enabled

Gamespeed Control
========================
The tools have full control over the gamespeed. The pause uses the gamespeed and sets it to 0. For 95% of usecases this will function as expected. However, in certain situations pausing the game will not __completely__ stop character movement. The character will move extremely slowly. This is particularly problematic if you are using IGCSDOF as the image is moving, which means you can never maintain focus.

However, as noted this is only in certain situations.

You can set a slow motion factor to reduce the game speed (to help with that perfect shot) and this can be used with the game pause i.e. you can enable slow motion and then use the game pause hotkey to pause the game.

Camera control device
========================
In the configuration tab of the IGCS Client, you can specify what to use for controlling the camera: 
controller, keyboard+mouse, or both. The device you pick is blocked from giving input to the game, 
if you press 'Numpad .' (On by default). 

About hotsampling support
==========================
There is hotsampling support in this game, however it comes with some caveats.
- Hotsampling canbe problematic and cause crashes. Use at your own risk and save often.
- You have to press set resolution twice - wait for the resolution to change and settle after the first press (if you have reshade loaded, wait for reshade to reload fully to reduce risk of a crash
- In the 1.1.1 update they added functionality which pauses the game to the inventory when you move focus away from the game window, which is what you need to do to hotsample. When using the game pause in these tools, this can lead to crashes as of v1.1.1. This was not a problem in the previous version of the game.
