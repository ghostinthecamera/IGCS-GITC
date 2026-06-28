GITS-IGCS Camera Tools
------
__The tools are entirely built on the awesome Injectable Game Camera System created by Frans Bouma. I have not really modified the core functionality in any substantial way but I have added a number of additions and refinements. You can find his patreon with even more tools based upon a new camera system with enhanced features, check it out!__  

All tool binaries have been removed as tools are being transferred to my new GITC Camera Tools platform - all currently available tools can be explored from https://ghostinthecamera.co.uk/tools/.

Patreon: [https://www.patreon.com/c/ghostinthecamera](https://www.patreon.com/ghostinthecamera/posts/deprecated-tools-162250868?utm_medium=clipboard_copy&utm_source=copyLink&utm_campaign=postshare_creator&utm_content=join_link)

### Cameras released: 
- Armored Core 6
- Granblue Fantasy: Relink
- Sekiro
- Final Fantasy XIII
- Final Fantasy XIII-2
- Lightning Returns: Final Fantasy XIII
- Dark Souls 3
- Dirt Rally 2.0
- GRID 2019

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


