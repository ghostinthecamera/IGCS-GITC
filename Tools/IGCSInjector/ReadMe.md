IGCSInjector
============================
Generic injector for camera dlls into game exes. 

This is a small tool to inject dlls (e.g. the IGCS based camera tools, but others as well) into running processes, e.g. games. While ExtremeInjector
or other injectors might work OK, chances are Windows Defender will simply block their usage, especially on Windows 10. Not with IGCSInjector, as it
apparently doesn't match Defender's checks so it's left alone. The tool doesn't do much, it simply allocates some memory inside the target exe's host process 
and loads the dll into that memory, and then starts the dll as if the host process has loaded the dll itself. Nothing nasty, it's actually designed windows
functionality (e.g. debuggers use this too). 

Release binaries: https://github.com/FransBouma/InjectableGenericCameraSystem/releases/tag/IGCSInjector_101   
Credits: Otis_Inf

### How to use
Configure the ini file by specifying the game exe and the dll to inject, then run the IGCSInjector.exe to inject the camera dll into the game's exe.

Example: 
To inject the hellblade tools into the hellblade game process at runtime, simply change the IGCSInjector.ini file to:
```ini
[InjectionData]
Process=HellbladeGame-Win64-Shipping.exe
Dll=HellbladeSenuasSacrificeCameraTools.dll
```

Run as administrator is preferable. Make sure you inject into the right process. It might be you start the game with `SomeGame.exe` but that .exe actually
runs `SuperDuperx64VersionOfGame.exe`, so you really have to inject into `SuperDuperx64VersionOfGame.exe`. 

Oh life is so simple, eh?
