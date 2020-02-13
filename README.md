Injectable Generic Camera System by Frans Bouma
===============================================

This is a generic injectable camera system which is used as a base for cameras for taking screenshots within games. 
The main purpose of the system is to hijack the in-game 3D camera by overwriting values in its camera structure
with our own values so we can control where the camera is located, it's pitch/yaw/roll values,
its FoV and the camera's look vector. Some camera implementations have additional features like timestop.

It's written in C++ with some x86/x64 assembler to be able to intercept the location of the 3D camera in the game. 
The system is initially designed for 64bit hosts as all games are 64bit nowadays, but has been reworked to be used for 32bit games too. 

## Folder structure description

In the folder `Cameras` you'll several implementations of the system, adapted for specific games. 

The cameras don't use a shared piece of code as in general cameras have to be adapted to a game pretty deeply and I didn't want to make a big
configurable ball. Additionally, cameras are often written once and perhaps fixed once or twice when the game is updated, but that's it. Copying
the code for each camera and adapting it makes possible to add new features to future cameras without affecting the older ones. 

## Requirements to build the code
To build the code, you need to have VC++ 2015 update 3 or higher, newer cameras need VC++ 2017. 
Additionally you need to have installed the Windows SDK, at least the windows 8 version. The VC++ installer should install this. 
The SDK is needed for DirectXMath.h

### External dependencies
There's an external dependency on [MinHook](https://github.com/TsudaKageyu/minhook) through a git submodule. This should be downloaded
automatically when you clone the repo. The camera uses DirectXMath for the 3D math, which is a self-contained .h file, from the Windows SDK. 

## Cameras released: 
Ace Combat 7: https://github.com/ghostinthecamera/IGCS-GITC/releases/tag/AC7v1.23  
Dirt Rally 2.0: https://github.com/ghostinthecamera/IGCS-GITC/releases/tag/DR2v2.11  
Mutant Year Zero: Road to Eden: https://github.com/ghostinthecamera/IGCS-GITC/releases/tag/MYZv1.1  
Deserts of Kharak: https://github.com/ghostinthecamera/IGCS-GITC/releases/tag/DoKv1.0  
BSG Deadlock (Reserruction DLC): https://github.com/ghostinthecamera/IGCS-GITC/releases/tag/BSGDv1.1  
Ni No Kuni: Wrath of the White Witch Remastered: https://github.com/ghostinthecamera/IGCS-GITC/releases/tag/NNKRv1.0  
Ni No Kuni 2: Revenant Kingdom: https://github.com/ghostinthecamera/IGCS-GITC/releases/tag/NNK2v1.0  
MechWarrior 5: Mercenaries: https://github.com/ghostinthecamera/IGCS-GITC/releases/tag/MW5v2.01  

## In-depth article about IGCS and how to create camera tools
Frans has written a long, in-depth article about how to create camera tools and how IGCS works [on his blog](https://weblogs.asp.net/fbouma/let-s-add-a-photo-mode-to-wolfenstein-ii-the-new-colossus-pc).

## Acknowledgements
Some camera code uses [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu.


