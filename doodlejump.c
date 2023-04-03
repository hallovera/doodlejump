/* This files provides address values that exist in the system */

#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BLACK 0x0000

#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 8
#define NUM_BOXES 8
	
#define NUM_COLOURS 10

#define FALSE 0
#define TRUE 1

#include <stdlib.h>
#include <stdio.h>
#include <time.h> //For use with rand()
#include <stdbool.h> //For use in draw line

// Begin part3.c code for Lab 7

volatile int pixel_buffer_start; // global variable

//FUNCTION DECLARATIONS
void clear_screen();
void draw_line (int x0, int y0, int x1, int y1, short int line_colour);
void swap (int *ptr_1, int *ptr_2);
void plot_pixel(int x, int y, short int line_color);
void erase (int boxes[8][NUM_BOXES]);
void draw_box (int x, int y, short int colour);
void draw(int boxes[8][NUM_BOXES], short int colours[NUM_BOXES], short int lineColours[NUM_BOXES]);
void wait_for_vsync();

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)
	
	/*SET THE SEED FOR RAND() so that it may be truly random*/
	time_t t;
	srand((unsigned)time(&t));
	
	/*COLOURS ARE STORES IN AN ARRAY SO THEY MAY BE RANDOMIZED
	*
	* INDEX 0: WHITE
	* INDEX 1: GREEN
	* INDEX 2: BLUE
	* INDEX 4: ORANGE
	* INDEX 5: YELLOW
	* INDEX 6: RED
	* INDEX 7: CYAN
	* INDEX 8: PINK
	* INDEX 9: GREY
	* INDEX 10: MAGENTA
	*
	*/
	
	short int colours[] = {WHITE, GREEN, BLUE, ORANGE, YELLOW, RED, CYAN, PINK, GREY, MAGENTA};
	
	//BUFFER SET UP
	
    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer
	
	//PROGRAM LOOP
	
	bool gameOver = false;
	bool keepPlaying = true;
	
	do 
    {
        
		
		
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    } while (keepPlaying)
}

// Code for subroutines (not shown)

//CLEAR SCREEN FUNCTION
void clear_screen() {
	
	//Nested for loop iterates through all of the pixels in the screen
    int x, y;
    for (x = 0; x < RESOLUTION_X; x++) {
		for (y = 0; y < RESOLUTION_Y; y++) {
			plot_pixel(x, y, BLACK); // Draw black to the pixel
		}
	}

}

//DRAW LINE FUNCTION USING BRESENHAM'S ALGORITHM
void draw_line (int x0, int y0, int x1, int y1, short int line_colour) {
	
	//bool is_steep = ABS(y1 - y0) > ABS(x1 - x0);
	
	bool is_steep = (abs(y1 - y0) > abs(x1 - x0));
  
	if (is_steep) {
		
		swap(&x0, &y0);
		swap(&x1, &y1);
		
	}
  
	if (x0 > x1) {
		
		swap(&x0, &x1);
		swap(&y0, &y1);

	}

	int deltax = x1 - x0;
	int deltay = abs(y1 - y0);
	
	int error = -1 * (deltax / 2);
	
	int y = y0;
  
	int y_step;
	
	if (y0 < y1) {  
		y_step = 1;
  	}
  
  	else { 
	  	y_step = -1;
  	}

  	for (int x = x0; x < (x1 + 1); x++) {
  		
		if (is_steep) {
			plot_pixel(y, x, line_colour);
		}
		
		else {
			plot_pixel(x, y, line_colour);
		}
		
		error = error + deltay;
		
		if (error > 0) {
			y = y + y_step;
			error = error - deltax;
		}
		
  	}
}

//SWAPS TWO VALUES WITH POINTERS
void swap (int *ptr_1, int *ptr_2) {
	//Store in temporary variable
	int temp = *ptr_2;
	
	//Swap the values using pointers
	*ptr_2 = *ptr_1;
	*ptr_1 = temp;
}

//Adds the pixel to the buffer
void plot_pixel(int x, int y, short int line_color) {
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

//FUNCTION THAT ESSENTIALLY DRAWS BLACK ON ALL BOXES AND LINES
void erase (int boxes[8][NUM_BOXES]) {
	//Iterate through the boxes
	int boxNum;
    for (boxNum = 0; boxNum < NUM_BOXES; boxNum++) {
		draw_box(boxes[4][boxNum], boxes[5][boxNum], BLACK);
		
		//if it is not the last box, connect to the one ahead of it
		if (boxNum != NUM_BOXES - 1) {
			draw_line(boxes[4][boxNum], boxes[5][boxNum], boxes[4][boxNum + 1], boxes[5][boxNum + 1], BLACK);
		}
		
		//if it is the last box, connect to the first box
		else {//if (boxNum == NUM_BOXES - 2) {
			//why did I have to use the previous 0th box to work?
            draw_line(boxes[6][0], boxes[7][0], boxes[4][boxNum], boxes[5][boxNum], BLACK);
		}
		
	}
}

//DRAWS THE BOX WITH A NESTED FOR LOOP
void draw_box (int x, int y, short int colour) {
	int x_pixel, y_pixel;
    for (x_pixel = 0; x_pixel < BOX_LEN; x_pixel++) {
		for (y_pixel = 0; y_pixel < BOX_LEN; y_pixel++) {
			plot_pixel(x + x_pixel, y + y_pixel, colour);
		}
	}
}

void draw(int boxes[8][NUM_BOXES], short int colours[NUM_BOXES], short int lineColours[NUM_BOXES]) {
		
	//Iterate through the boxes
	int boxNum;
    for (boxNum = 0; boxNum < NUM_BOXES; boxNum++) {
		
		//Swap the second previous with the most previous
		boxes[4][boxNum] = boxes[6][boxNum];
		boxes[5][boxNum] = boxes[7][boxNum];
		
		//Update the previous positions
		boxes[6][boxNum] = boxes[0][boxNum];
		boxes[7][boxNum] = boxes[1][boxNum];
		
		//Draw the box
		draw_box(boxes[0][boxNum], boxes[1][boxNum], colours[boxNum]);
		
		//if it is not the last box, connect to the one ahead of it
		if (boxNum != NUM_BOXES - 1) {
			draw_line(boxes[0][boxNum], boxes[1][boxNum], boxes[0][boxNum + 1], boxes[1][boxNum + 1], lineColours[boxNum]);
		}
		
		//if it is the last box, connect to the first box
		else {//if (boxNum == NUM_BOXES - 1) {
			draw_line(boxes[0][0], boxes[1][0], boxes[0][boxNum], boxes[1][boxNum], lineColours[boxNum]);
		}
		
		//CHECK FOR EDGE CONDITIONS
		
		//if the box is about to go off the screen in x direction
		if (((boxes[0][boxNum] == RESOLUTION_X - 1 - BOX_LEN) && (boxes[2][boxNum] == 1)) || ((boxes[0][boxNum] == 0) && (boxes[2][boxNum] == -1))) {
			boxes[2][boxNum] *= -1;
		}
		
		//if the box is about to go off the screen in y direction
		if (((boxes[1][boxNum] == RESOLUTION_Y - 1 - BOX_LEN) && (boxes[3][boxNum] == 1)) || ((boxes[1][boxNum] == 0) && (boxes[3][boxNum] == -1))) {
			boxes[3][boxNum] *= -1;
		}
		
		//CHANGE THE POSITION
		
		boxes[0][boxNum] += boxes[2][boxNum];//Step the x position
		boxes[1][boxNum] += boxes[3][boxNum];//Step the y position
		
	}
}

//Vsync delay function as given in lecture
void wait_for_vsync() {
	volatile int * pixel_ctrl_ptr = 0xFF203020;//PIXEL CONTROLLER
	register int status;
	
	*pixel_ctrl_ptr = 1; //Start the synchronization process
	
	status = *(pixel_ctrl_ptr + 3); //Read the status register
	
	while ((status & 0x01) != 0) {
	
		status = *(pixel_ctrl_ptr + 3);//poll the s bit
		
	}
	
}
	
	