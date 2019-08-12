Injectable camera for Assassin's Creed Odyssey
============================

Current supported game version: v1.4.0+ (UPlay version)  
Camera version: 1.0.8  
Camera release binaries: https://github.com/FransBouma/InjectableGenericCameraSystem/releases/tag/ACOD_108  
Credits: Otis_Inf. 

### Features

- Camera control: (Also in cut scenes and during a paused game)
	- FoV control
	- Free unlimited camera movement and rotation 
- Game pause / unpause, also in cut scenes. 
- Time of Day control
- Resolution scaling
- Photomode range limiter removal
- HUD toggle
- Disable in-game DoF when camera is enabled. (will re-enable when you disable camera).
- Aspect Ratio limitations have been removed.
- Fog control (much higher than photomode allows)

### Change log:
v1.0.8: Better AOB scanner added which can deal better with DRM driven block moves. When enabling the camera, in-game DOF is now properly disabled, 
it's no longer necessary to enable the camera menu to disable in-game dof. 
v1.0.7: Fix for 1.4.0 of the game.
v1.0.6: Fixed ToD interception
v1.0.5: Fix for 1.1.4 of the game. 
v1.0.4: Multiple actions bound to the same key are now supported.
v1.0.3: Configurable keybindings added.
v1.0.2: Fog control added. Photomode goes up to 10, we go up to 200 (manual override is possible: ctrl-click and type a value). 
v1.0.1: Fix for 1.0.7 of the game: photomode range limit was broken. Added: AR limits removed 

### How to use
Read the enclosed readme.txt for details, really! Run the game as **administrator** otherwise it won't work.

### Important
* Be careful with the resolution scaling factor in the camera tools settings. Using a value of 2 or higher with a very 
high resolution will likely make the game become unresponsive and crash if you don't have the latest greatest videocard.
Resolution scaling already creates a high-res framebuffer, so e.g. using a factor of 2.0 on a 5K resolution effectively
means the game renders a 10K image, something it won't be able to do, most likely.

* These camera tools were made with and for the UPlay version of the game. If you use it on the Steam version, it might not
work, though I expect it to work OK as the code likely is the same so the AOB scans will find the code regardless. If it doesn't
work, then... too bad, sorry.

### EULA
To use these camera tools, you have to comply to the following:
If you ask me a question which is answered in the enclosed readme.txt, so i.o.w. you didn't read it at all, 
you owe me a new AAA game. Easy, right? 

### Acknowledgements
This camera uses [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu.
