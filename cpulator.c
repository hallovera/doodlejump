/* This files provides address values that exist in the system */

#define BOARD                 "DE1-SoC"

/* Memory */
#define DDR_BASE              0x00000000
#define DDR_END               0x3FFFFFFF
#define A9_ONCHIP_BASE        0xFFFF0000
#define A9_ONCHIP_END         0xFFFFFFFF
#define SDRAM_BASE            0xC0000000
#define SDRAM_END             0xC3FFFFFF
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_ONCHIP_END       0xC803FFFF
#define FPGA_CHAR_BASE        0xC9000000
#define FPGA_CHAR_END         0xC9001FFF

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define JP1_BASE              0xFF200060
#define JP2_BASE              0xFF200070
#define PS2_BASE              0xFF200100
#define PS2_DUAL_BASE         0xFF200108
#define JTAG_UART_BASE        0xFF201000
#define JTAG_UART_2_BASE      0xFF201008
#define IrDA_BASE             0xFF201020
#define TIMER_BASE            0xFF202000
#define AV_CONFIG_BASE        0xFF203000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030
#define AUDIO_BASE            0xFF203040
#define VIDEO_IN_BASE         0xFF203060
#define ADC_BASE              0xFF204000

/* Cyclone V HPS devices */
#define HPS_GPIO1_BASE        0xFF709000
#define HPS_TIMER0_BASE       0xFFC08000
#define HPS_TIMER1_BASE       0xFFC09000
#define HPS_TIMER2_BASE       0xFFD00000
#define HPS_TIMER3_BASE       0xFFD01000
#define FPGA_BRIDGE           0xFFD0501C

/* ARM A9 MPCORE devices */
#define   PERIPH_BASE         0xFFFEC000    // base address of peripheral devices
#define   MPCORE_PRIV_TIMER   0xFFFEC600    // PERIPH_BASE + 0x0600

/* Interrupt controller (GIC) CPU interface(s) */
#define MPCORE_GIC_CPUIF      0xFFFEC100    // PERIPH_BASE + 0x100
#define ICCICR                0x00          // offset to CPU interface control reg
#define ICCPMR                0x04          // offset to interrupt priority mask reg
#define ICCIAR                0x0C          // offset to interrupt acknowledge reg
#define ICCEOIR               0x10          // offset to end of interrupt reg
/* Interrupt controller (GIC) distributor interface(s) */
#define MPCORE_GIC_DIST       0xFFFED000    // PERIPH_BASE + 0x1000
#define ICDDCR                0x00          // offset to distributor control reg
#define ICDISER               0x100         // offset to interrupt set-enable regs
#define ICDICER               0x180         // offset to interrupt clear-enable regs
#define ICDIPTR               0x800         // offset to interrupt processor targets regs
#define ICDICFR               0xC00         // offset to interrupt configuration regs

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
	
//Constants for IRQ Handler
#define PS2_IRQ              0x4F //IR ID for PS2

#define IRQ_MODE			0b10010
#define SVC_MODE			0b10011

#define INT_ENABLE			0b01000000
#define INT_DISABLE			0b11000000

#define ENABLE 0x1

void set_A9_IRQ_stack(void);
void config_GIC(void);
void config_HPS_timer(void);
void config_HPS_GPIO1(void);
void config_interval_timer(void);
void config_KEYs(void);
void enable_A9_interrupts(void);

//Lab 5 Functions
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

/* function prototypes */
void HEX_PS2(char, char, char);

void PS2_ISR(void);

void config_PS2();

/* key_dir and pattern are written by interrupt service routines; we have to
 * declare these as volatile to avoid the compiler caching their values in
 * registers */
volatile int tick = 0; // set to 1 every time the HPS timer expires
volatile int key_dir = 0;
volatile int pattern = 0x0F0F0F0F; // pattern for LED lights
/* ********************************************************************************
 * This program demonstrates use of interrupts with C code. It first starts
 * two timers: an HPS timer, and the FPGA interval timer. The program responds
 * to interrupts from these timers in addition to the pushbutton KEYs in the
 * FPGA.
 *
 * The interrupt service routine for the HPS timer causes the main program to
 * flash the green light connected to the HPS GPIO1 port.
 *
 * The interrupt service routine for the interval timer displays a pattern on
 * the LED lights, and shifts this pattern either left or right. The shifting
 * direction is reversed when KEY[1] is pressed
 ********************************************************************************/
int main(void)
{
    //volatile int *HPS_GPIO1_ptr = (int *)HPS_GPIO1_BASE; // GPIO1 base address
    //volatile int HPS_timer_LEDG =
    //    0x01000000;          // value to turn on the HPS green light LEDG
    set_A9_IRQ_stack();      // initialize the stack pointer for IRQ mode
    config_GIC();            // configure the general interrupt controller
    //config_HPS_timer();      // configure the HPS timer
    //config_HPS_GPIO1();      // configure the HPS GPIO1 interface
    //config_interval_timer(); // configure Altera interval timer to generate
	config_PS2();
    // interrupts
    config_KEYs();          // configure pushbutton KEYs to generate interrupts
    enable_A9_interrupts(); // enable interrupts
	
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
	
	/*THE BOXES WILL THE STORED IN A DOUBLE ARRAY OF INTEGERS
	* The first array holds an array of integers
	* The second array of integers holds
	* x coord of box
	* y coord of box
	* x step of box
	* y step of box
	* x previous
	* y previous
	* x double previous
	* y double previous
	*/
	
	int boxes [8][NUM_BOXES];
	short int boxColours[NUM_BOXES];
    short int lineColours[NUM_BOXES];
	
	//Now let's initalize the boxes with rand()
	
	int boxNum;
    for (boxNum = 0; boxNum < NUM_BOXES; boxNum++) {
		//Set x coord
		boxes[0][boxNum] = rand() % (RESOLUTION_X - BOX_LEN);
		
		//Set y coord
		boxes[1][boxNum] = rand() % (RESOLUTION_Y - BOX_LEN); 
			
		
		//Set x step
		boxes[2][boxNum] = rand() % 2 * 2 - 1;
			
		
		//Set y step
		boxes[3][boxNum] = rand() % 2 * 2 - 1;
		
		//Set x previous
		boxes[4][boxNum] = 0;//boxes[0][boxNum];
		
		//Set y previous
		boxes[5][boxNum] = 0;//boxes[1][boxNum];
		
		//Set x double previous
		boxes[6][boxNum] = 0;//boxes[0][boxNum];
		
		//Set y double previous
		boxes[7][boxNum] = 0;//boxes[1][boxNum];
			
			
		//Set colour
		boxColours[boxNum] = colours[rand() % NUM_COLOURS];
		
        //Set line colours
        lineColours[boxNum] = colours[rand() % NUM_COLOURS];

	}

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
	
	while (1)
    {
        
		// Erase any boxes and lines that were drawn in the last iteration 
        erase(boxes);
		//clear_screen();

        // code for drawing the boxes and lines (not shown)
        // code for updating the locations of boxes (not shown)
		draw(boxes, boxColours, lineColours);
		
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
	    
}

void PS2_ISR(void)
{
    /* Declare volatile pointers to I/O registers (volatile means that IO load
    and store instructions will be used to access these pointer locations,
    instead of regular memory loads and stores) */
    volatile int *PS2_ptr = (int *)PS2_BASE;
    int PS2_data, RVALID;
    char byte1 = 0, byte2 = 0, byte3 = 0;

    PS2_data = *(PS2_ptr);      // read the Data register in the PS/2 port
    RVALID = PS2_data & 0x8000; // extract the RVALID field

    volatile int *LEDR_ptr = (int *)LEDR_BASE;

    if (RVALID)
    {
        *(LEDR_ptr) = PS2_data;
		//*(LEDR_ptr) = 1;
    }
}

/****************************************************************************************
 * Subroutine to show a string of HEX data on the HEX displays
 ****************************************************************************************/
void HEX_PS2(char b1, char b2, char b3)
{
    volatile int *HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
    volatile int *HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;
    /* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
     * a single 7-seg display in the DE1-SoC Computer, for the hex digits 0 - F
     */
    unsigned char seven_seg_decode_table[] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
        0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
    unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned int shift_buffer, nibble;
    unsigned char code;
    int i;
    shift_buffer = (b1 << 16) | (b2 << 8) | b3;
    for (i = 0; i < 6; ++i)
    {
        nibble = shift_buffer & 0x0000000F; // character is in rightmost nibble
        code = seven_seg_decode_table[nibble];
        hex_segs[i] = code;
        shift_buffer = shift_buffer >> 4;
    }
    /* drive the hex displays */
    *(HEX3_HEX0_ptr) = *(int *)(hex_segs);
    *(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);
}

// Define the IRQ exception handler
void __attribute__((interrupt)) __cs3_isr_irq(void)
{
    // Read the ICCIAR from the processor interface
    int address = MPCORE_GIC_CPUIF + ICCIAR;
    int int_ID = *((int *)address);
    if (int_ID == PS2_IRQ) {
        PS2_ISR();
    }
    else
        while (1)
            ; // if unexpected, then stay here
    // Write to the End of Interrupt Register (ICCEOIR)
    address = MPCORE_GIC_CPUIF + ICCEOIR;
    *((int *)address) = int_ID;
    return;
}

/*Set up the PS2 to work with interrupts*/
void config_PS2() {
    volatile int *PS2_ptr = (int *)PS2_BASE;

    //Clear the keyboard
    *(PS2_ptr) = 0xFF; //reset the keyb

    //Enable the 0th bit to have interrupts
    *(PS2_ptr + 1) = 0x0001;

}

/* setup HPS timer */
void config_HPS_timer()
{
    volatile int *HPS_timer_ptr = (int *)HPS_TIMER0_BASE; // timer base address
    *(HPS_timer_ptr + 0x2) = 0;                           // write to control register to stop timer
    /* set the timer period */
    int counter = 100000000;    // period = 1/(100 MHz) x (100 x 10^6) = 1 sec
    *(HPS_timer_ptr) = counter; // write to timer load register
    /* write to control register to start timer, with interrupts */
    *(HPS_timer_ptr + 2) = 0b011; // int mask = 0, mode = 1, enable = 1
}
/* setup HPS GPIO1. The GPIO1 port has one green light (LEDG) and one pushbutton
 * KEY connected for the DE1-SoC Computer. The KEY is connected to GPIO1[25],
 * and is not used here. The green LED is connected to GPIO1[24]. */
void config_HPS_GPIO1()
{
    volatile int *HPS_GPIO1_ptr = (int *)HPS_GPIO1_BASE; // GPIO1 base address
    *(HPS_GPIO1_ptr + 0x1) =
        0x01000000; // write to the data direction register to set
    // bit 24 (LEDG) to be an output
    // Other possible actions include setting up GPIO1 to use the KEY, including
    // setting the debounce option and causing the KEY to generate an interrupt.
    // We do not use the KEY in this example.
}
/* setup the interval timer interrupts in the FPGA */
void config_interval_timer()
{
    volatile int *interval_timer_ptr =
        (int *)TIMER_BASE; // interal timer base address
    /* set the interval timer period for scrolling the HEX displays */
    int counter = 5000000; // 1/(100 MHz) x 5x10^6 = 50 msec
    *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
    *(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;
    /* start interval timer, enable its interrupts */
    *(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1
}
/* setup the KEY interrupts in the FPGA */
void config_KEYs()
{
    volatile int *KEY_ptr = (int *)KEY_BASE; // pushbutton KEY address
    *(KEY_ptr + 2) = 0x3;                    // enable interrupts for KEY[1]
}
/*
 * Initialize the banked stack pointer register for IRQ mode
 */
void set_A9_IRQ_stack(void)
{
    int stack, mode;
    stack = A9_ONCHIP_END - 7; // top of A9 onchip memory, aligned to 8 bytes
    /* change processor to IRQ mode with interrupts disabled */
    mode = INT_DISABLE | IRQ_MODE;
    asm("msr cpsr, %[ps]"
        :
        : [ps] "r"(mode));
    /* set banked stack pointer */
    asm("mov sp, %[ps]"
        :
        : [ps] "r"(stack));
    /* go back to SVC mode before executing subroutine return! */
    mode = INT_DISABLE | SVC_MODE;
    asm("msr cpsr, %[ps]"
        :
        : [ps] "r"(mode));
}
/*
 * Turn on interrupts in the ARM processor
 */
void enable_A9_interrupts(void)
{
    int status = SVC_MODE | INT_ENABLE;
    asm("msr cpsr, %[ps]"
        :
        : [ps] "r"(status));
}
/*
 * Configure the Generic Interrupt Controller (GIC)
 */
void config_GIC(void)
{
    int address; // used to calculate register addresses
    /* configure the HPS timer interrupt */
    *((int *)0xFFFED8C4) = 0x01000000;
    *((int *)0xFFFED118) = 0x00000080;
    /* configure the FPGA interval timer and KEYs interrupts */
    *((int *)0xFFFED848) = 0x00000101;
    *((int *)0xFFFED108) = 0x00000300;
    // Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all
    // priorities
    address = MPCORE_GIC_CPUIF + ICCPMR;
    *((int *)address) = 0xFFFF;
    // Set CPU Interface Control Register (ICCICR). Enable signaling of
    // interrupts
    address = MPCORE_GIC_CPUIF + ICCICR;
    *((int *)address) = ENABLE;
    // Configure the Distributor Control Register (ICDDCR) to send pending
    // interrupts to CPUs
    address = MPCORE_GIC_DIST + ICDDCR;
    *((int *)address) = ENABLE;
}

// Define the remaining exception handlers
void __attribute__((interrupt)) __cs3_reset(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_undef(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_swi(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_pabort(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_dabort(void)
{
    while (1)
        ;
}
void __attribute__((interrupt)) __cs3_isr_fiq(void)
{
    while (1)
        ;
}


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
