# xhblib

Xbox 360 Homebrew Library for easier interfacing with the console, for homebrew projects using the official XDK.

Use with Visual Studio 2010 Ultimate (i'm working oon a thing that makes this not a requirement) and XDK 21256.3

## How to add to your project?:

Put it inside the solution folder, then in visual studio right-click your solution, add, existing project, go to xhblib directory and double-click the xhblib.vcxproj file. 
If the linker reports unresolved functions, add the xhblib.lib file to your project: Project → Properties → Linker → Input → Additional Dependencies.

## If you have any feature request, or bug fixes, make a pull request or a issue

## Changelog (from initial commit):
Added Filesystem Wrapper

Added launching .xex's

Updated XboxPAD:

    Added vibration
    
    Added reading trigger values
    
    Added reading stick axis values

## Planned:
Optimize XD2D (Just write to the framebuffer, omit D3D. Help needed)

Drawing using TTF fonts

Drawing vector graphics
