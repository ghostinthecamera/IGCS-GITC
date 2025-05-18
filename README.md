GITS-IGCS Camera Tools
------
__The tools are entirely built on the awesome Injectable Game Camera System created by Frans Bouma. I have not really modified his work in any substantial way. You can find his patreon with even more tools based upon a new camera system with enhanced features, check it out!__  

Update: I am currently working on updating older cameras to the newer codebase and as such they will be removed from the public repo while that happens. 
Releases that are working will still be available, but these will also be replaced with the most up to date version.

These tools hijack the in-game camera and allow control over the camera's position and rotation. The tools also 
allow control of the field of view. Some camera implementations have additional features such as the ability to control the game speed.

Making a camera for a game takes a lot of my personal free time (even thought Frans' system is most of the work done!). This isn't my day job unfortunately! Therefore, if you have used these tools
and want to show your appreciation or just a quick thank you then please drop me a message or if you're feeling extra generous, you can head over to my Ko-Fi  
  
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/M4M0VZFCD)

### Cameras released: 
- Armored Core 6
- Granblue Fantasy: Relink
- Sekiro
- Final Fantasy XIII
- Final Fantasy XIII-2
- Lightning Returns: Final Fantasy XIII
- Dark Souls 3

---


### Requirements to build the code
To build the code, you need to have VC++ 2015 update 3 or higher, newer cameras need VC++ 2017. 
Additionally you need to have installed the Windows SDK, at least the windows 8 version. The VC++ installer should install this. 
The SDK is needed for DirectXMath.h

### External dependencies
There's an external dependency on [MinHook](https://github.com/TsudaKageyu/minhook) through a git submodule. This should be downloaded
automatically when you clone the repo. The camera uses DirectXMath for the 3D math, which is a self-contained .h file, from the Windows SDK. 

### Acknowledgements
Some camera code uses [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu.


