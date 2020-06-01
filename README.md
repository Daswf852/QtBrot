# QtBroth
A mandelbrot implementation with a few methods for calculating written in C++14 using Qt, SFML and OpenCL  
  
## Building
Firstly install the necessary Qt, SFML and OpenCL packages.  
Then do:  
`mkdir build && cd build`  
`qmake ..`  
`make`  
  
## Running
Just execute the built binary.  
To use OpenCL, Choose a kernel first. You can use the included `kernel.cl` or you can write your own.  
GUI sometimes hangs (Probably because I call UpdateUI() a tad bit too much :/ ). Buttons etc. will still work but you might want to restart the thing.  
  
## To-Do
 - De-spaghettify the code. Especially the OpenCL worker and all its related stuff  
 - Add "teleportation"  
 - Find a purpose for that placeholder checkbox because the "draw while updating" checkbox alone looks lonely  
