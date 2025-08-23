# xhblib

Xbox 360 Homebrew Library made to be easy to use in homebrew projects, using the official XDK

For use with Visual Studio 2010 Ultimate and XDK 21256.3

## How to add to your project?:

Put it inside the solution folder, then in visual studio right-click your solution, add, existing project, go to xhblib directory and double-click the xhblib.vcxproj file. 
If the linker reports unresolved functions, add the xhblib.lib file to your project: Project → Properties → Linker → Input → Additional Dependencies.

## If you have any feature request, or bug fixes, make a pull request or a issue

## What's new?
Added Filesystem Wrapper

Updated XboxPAD:

    Added vibration
    
    Added reading trigger values
    
    Added reading stick axis values

## Planned Features:
Drawing using TTF fonts

Drawing vector graphics

Reading keyboard presses

Launching XEX's
