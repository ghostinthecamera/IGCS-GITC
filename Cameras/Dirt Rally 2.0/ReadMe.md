Injectable camera for Devil May Cry 5
============================

Current supported game version: v1.0+
Camera version: 1.0.1  
Camera release binaries: https://github.com/FransBouma/InjectableGenericCameraSystem/releases/tag/DMC5_101  
Credits: Otis_Inf, Jim2Point0, Hattiwatti, K-putt. 

Brought to you by [FRAMED. Screenshotting community](https://framedsc.github.io). 

![](https://framedsc.github.io/Images/FRAMED_LogoBigDarkTransparent800px.png)

### Features

- Camera control: (Also in cut scenes and during a paused game)
	- FoV control
	- Free unlimited camera movement and rotation 
- Game pause / unpause, also in cut scenes. 
- Resolution scaling
- Hud toggle
- Aspect Ratio selection.
- DoF removal in cutscenes.

### Important: DirectX 11 only
The tools only work with DirectX11 of the game. The game itself starts with DirectX 12 by default. To switch back
to DirectX 11, please see [this guide](https://framedsc.github.io/GameGuides/devil_may_cry_5.htm).

### How to use
Read the enclosed readme.txt for details, really! Run the game as **administrator** otherwise it won't work.

### Important
* Be careful with the resolution scaling factor in the camera tools settings. Using a value of 4-5 or higher with a very 
high resolution will likely make the game become unresponsive and crash if you don't have the latest greatest videocard.
Resolution scaling already creates a high-res framebuffer, so e.g. using a factor of 2.0 on a 5K resolution effectively
means the game renders a 10K image, something it won't be able to do, most likely.

### EULA
To use these camera tools, you have to comply to the following:
If you ask me a question which is answered in the enclosed readme.txt, so i.o.w. you didn't read it at all, 
you owe me a new AAA game. Easy, right? 

### Acknowledgements
This camera uses [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu.
