# genshin-auto-dialogue

## Overview
Automatically choose options and fast forward dialogue playing in genshin impact windows version.

Same concept with [1hubert/genshin-dialogue-autoskip](https://github.com/1hubert/genshin-dialogue-autoskip), this program is written in Modern C++ for better performance and lesser memory footprint. Process can keep running on background without hampering gaming performance.

Match two patterns in dialogue playing:
![demo](https://github.com/kingsidelee/genshin-auto-dialogue/blob/main/demo.png)

## How to use
- Download prebuilt executable from release or build for yourself.
- Right click on executable and choose "Run as administrator".
- Open the game on your primary monitor and enjoy.

## How to build
- MinGW one liner: `g++ -std=c++17 -O3 .\genshin-auto-dialogue.cpp -o .\genshin-auto-dialogue.exe -lgdi32`
- MinGW static build: `g++ -std=c++17 -O3 .\genshin-auto-dialogue.cpp -o .\genshin-auto-dialogue.exe -lgdi32 -static-libgcc -static-libstdc++`

## Requirements
- Program use Windows API to simulate mouse and keyboard event
- Support 16:9 resolutions: 1920x1080, 2560x1440, 3840x2160. Other resolutions can be achieve by modifying pixel coordinates.
- The executable have to run as admin because [User Interface Privilege Isolation](https://en.wikipedia.org/wiki/User_Interface_Privilege_Isolation).

## Resource usage
After few minutes of playing on a 3840x2160 monitor:
```
PS > Get-Process -Name 'genshin-auto-dialogue'

Handles  NPM(K)    PM(K)      WS(K)     CPU(s)     Id  SI ProcessName
-------  ------    -----      -----     ------     --  -- -----------
     55       6    33380      37300       0.41   9360   1 genshin-auto-dialogue
```

## Bugs and limitations
- Due to the nature of asynchronous threads, there is a delay between next screenshot and stopping the keyboard event.
- Default screenshot interval is 3 seconds, this can be tuned to fit your setup.
