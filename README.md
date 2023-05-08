# Doodle Jump

## Description
This is an implementation of the popular mobile game [Doodle Jump](https://play.google.com/store/apps/details?id=com.lima.doodlejump&hl=en&gl=US&pli=1), written using C and ARM Assembly for the [DE1-SoC board](https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=836). It was made to be a rather *low-level* implementation, meaning it doesn't use graphics libraries; we interact directly with memory-mapped peripherals through their memory addresses, and draw graphics pixel-by-pixel using techniques like [Bresenham's line algorithm](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm). The game itself is displayed onto a VGA display and controlled using a custom PS/2 keyboard.

The final version of our code is in ```doodlejump_final.c``` (due to certain software limitations, we had to put all our code into a single file).

## Purpose and Results
This project helped us learn a lot about embedded and low-level hardware programming, as well as computer organization. This included: how to interface directly with computer peripherals like keyboards and monitors using memory-mapped I/O; how to translate Assembly into high-level languages like C (and vice versa); how to use hardware interrupts in both Assembly and C to control the game and handle user input; and how to actively create and erase graphics at the most granular, pixel-by-pixel level, without the help of any graphics libraries whatsoever, and then have it displayed on a monitor in real-time. 

## Instructions
The original demonstration was done in-person on a physical VGA monitor, which can't be done anymore. However, you can still demo the game using [CPULator](https://cpulator.01xz.net/?sys=arm-de1soc), an ARM processor emulator. Follow these steps:
1. Download the ```doodlejump_final.c``` file.
2. Go to [CPULator](https://cpulator.01xz.net/?sys=arm-de1soc).
3. Click ```File > Open > ``` (Open the ```doodlejump_final.c``` file you downloaded). The code will show up in the editor. 
4. Click ```Compile and Load (F5)```. Wait for it to indicate completion in the ```Messages``` tab at the bottom.
5. On the right sidebar labelled ```Devices```, scroll down until you see ```VGA Pixel Buffer```; this is the **screen** that the game will display on. You can pop this out by clicking on the top-left dropdown and selecting ```Show in a Separate Box```.
6. On the right sidebar again, scroll down until you see ```PS2 Keyboard or Mouse```. This is how you'll **control** the game. Click the ```Type Here``` box; when the game starts, use the the left/right arrow keys on your keyboard to move your character left/right, and the up key to shoot. You can also pop this box out, using the same method as above.
7. On the top bar, click ```Continue``` to run the game. You may need to click back into the ```Type Here``` box to control your character. Enjoy!
