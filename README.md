"# Arduboy-Super-Sprint" 

My attempt at making a Super Sprint style game on the Arduboy.

<img width="128" height="64" alt="metrodromeDrifter_Lap" src="https://github.com/user-attachments/assets/1f8280b4-6c9a-4ce0-b737-9f1fe7b87ffc" />


<img width="128" height="64" alt="motodromeDrifter_Overview" src="https://github.com/user-attachments/assets/0a065d11-9f8c-4de7-a6cb-8b1a8c169fdb" />


V1.0 Now finished!

Play in browser (note, it plays better on the Arduboy controls):

https://tiberiusbrown.github.io/Ardens/?file=https://github.com/gary909/Arduboy-Super-Sprint/releases/download/v1.0.0/MotodromeDrifter.arduboy

<img width="404" height="267" alt="image" src="https://github.com/user-attachments/assets/d502289b-3a20-4a38-bb64-b8fd0b6bfb32" />


Arduboy hex files and .arduboy files can be found here:

https://github.com/gary909/Arduboy-Super-Sprint/releases/tag/v1.0.0


_____________DEV LOG_____________

V17 - fixed bugs on track 3/4 checkpoint

V16 - removed track. improvements

V15 - refined HUD positions

<img width="300" height="398" alt="IMG_20260830_125425815" src="https://github.com/user-attachments/assets/82a6a38a-cb8d-4e0e-94e9-1cd749137347" />


V14 - Fixed 2-Player Score Update Timing & UI Polish

V13 - Dynamic Memory Optimization, reduced from 79% to 62%

V12 - Complete 2-Player Pass & Play with Overall League Champion Display

V11 - Refactored Main Menu, 2-Player Pass & Play, Level Select, and HUD Guidance

<img width="128" height="64" alt="motodromeDrifter_Titles" src="https://github.com/user-attachments/assets/225f49ee-4d87-4073-a229-47eb35336154" />


V10 - courses refined. pretty good now. + drunk

V9 - Added flashing highscore

______________________________________
Tools for level creation:

Draw the level and then copy the coordinates.  You can try it out here here:

[https://github.com/gary909/Canvas-Visualiser](https://gary909.github.io/Canvas-Visualiser/)

<img width="480" height="488" alt="canvasDrawer-ezgif com-optimize" src="https://github.com/user-attachments/assets/715601fb-57d3-40d5-ab4f-3d8cd7fff5fa" />

Copy the cordinates from the app, into the .ino code (replacing 'level 2' etc). 

Also created a single level version of the code so you can quickly prototype new level designs:

https://github.com/gary909/Arduboy-Super-Sprint-Single-Level-/blob/main/README.md

______________________________________

V8 - Added Course 2 support, Course 2 track walls, dynamic start/checkpoints, and EEPROM support for Course 2 scores

<img width="300" height="398" alt="IMG_20260826_173818193 (1)" src="https://github.com/user-attachments/assets/dd071701-15e3-4cad-97ce-38c6465ac902" />

<img width="300" height="398" alt="IMG_20260826_173746420 (1)" src="https://github.com/user-attachments/assets/fa63dd8b-6913-4aa7-8a1b-de5d773b4830" />


V7 - Added Race Intro screen ("RACE 1/8") before track loads

V6 - Added Top 5 Highscore Screen with EEPROM storage & 2-second finish delay

V5 - Added Title Screen & Initials Entry System

<img width="300" alt="arduboy screen v5 1" src="https://github.com/user-attachments/assets/e8c3dd88-3371-4d24-8929-096f4f3b961f" />


<img width="300" alt="arduboy screen v5 2" src="https://github.com/user-attachments/assets/64f00393-a6a0-4163-bad7-242c7ef0028e" />


V4 - Added Timer, lap counter

<img width="300" height="206" alt="arduboytext v4" src="https://github.com/user-attachments/assets/a2cc6743-3f00-4560-949f-1d6430877b53" />


V3 - Starting to add courses / Added Up button for Brake and Down button for reverse

<img width="300" height="256" alt="arduboy screen v3 1" src="https://github.com/user-attachments/assets/67a29b61-15a4-464d-8c77-ae711a13124f" />


FYI Display Details 

Width: 128 pixels - Height: 64 pixels

<img width="602" height="228" alt="arduboy screen v3" src="https://github.com/user-attachments/assets/c85a4640-d6c7-4c3b-bab2-8d40a8e0cc52" />


V2 - Drift physics added to B Button


V1:


<img width="300" height="398" alt="arduboy screen v1" src="https://github.com/user-attachments/assets/d5899413-c706-454f-a5b2-7e61cb95490c" />
