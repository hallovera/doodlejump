void set_A9_IRQ_stack(void);
void config_GIC(void);
void config_HPS_timer(void);
void config_HPS_GPIO1(void);
void config_interval_timer(void);
void config_KEYs(void);
void enable_A9_interrupts(void);

/* function prototypes */
void HEX_PS2(char, char, char);

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
    volatile int *HPS_GPIO1_ptr = (int *)HPS_GPIO1_BASE; // GPIO1 base address
    volatile int HPS_timer_LEDG =
        0x01000000;          // value to turn on the HPS green light LEDG
    set_A9_IRQ_stack();      // initialize the stack pointer for IRQ mode
    config_GIC();            // configure the general interrupt controller
    config_HPS_timer();      // configure the HPS timer
    config_HPS_GPIO1();      // configure the HPS GPIO1 interface
    config_interval_timer(); // configure Altera interval timer to generate
    // interrupts
    config_KEYs();          // configure pushbutton KEYs to generate interrupts
    enable_A9_interrupts(); // enable interrupts
    while (1)
    {
        if (tick)
        {
            tick = 0;
            *HPS_GPIO1_ptr = HPS_timer_LEDG; // turn on/off the green light LEDG
            HPS_timer_LEDG ^= 0x01000000;    // toggle the bit that controls LEDG
        }
    }

    /* Declare volatile pointers to I/O registers (volatile means that IO load
and store instructions will be used to access these pointer locations,
instead of regular memory loads and stores) */
    volatile int *PS2_ptr = (int *)PS2_BASE;
    int PS2_data, RVALID;
    char byte1 = 0, byte2 = 0, byte3 = 0;
    // PS/2 mouse needs to be reset (must be already plugged in)
    *(PS2_ptr) = 0xFF; // reset
    while (1)
    {
        PS2_data = *(PS2_ptr);      // read the Data register in the PS/2 port
        RVALID = PS2_data & 0x8000; // extract the RVALID field
        if (RVALID)
        {
            /* shift the next data byte into the display */
            byte1 = byte2;
            byte2 = byte3;
            byte3 = PS2_data & 0xFF;
            HEX_PS2(byte1, byte2, byte3);
            if ((byte2 == (char)0xAA) && (byte3 == (char)0x00))
                // mouse inserted; initialize sending of data
                *(PS2_ptr) = 0xF4;
        }
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
    if (int_ID == HPS_TIMER0_IRQ) // check if interrupt is from the HPS timer
        HPS_timer_ISR();
    else if (int_ID ==
             INTERVAL_TIMER_IRQ) // check if interrupt is from the Altera timer
        interval_timer_ISR();
    else if (int_ID == KEYS_IRQ) // check if interrupt is from the KEYs
        pushbutton_ISR();
    else
        while (1)
            ; // if unexpected, then stay here
    // Write to the End of Interrupt Register (ICCEOIR)
    address = MPCORE_GIC_CPUIF + ICCEOIR;
    *((int *)address) = int_ID;
    return;
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
