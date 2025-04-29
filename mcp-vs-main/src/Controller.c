//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Controller.h"
#include "C:\Users\jackg\Desktop\University\2.1\MXEN2003\mcp-vs-main\lib\adc\adc.h" //minimal adc lib

volatile uint8_t databyte1 = 0;
volatile uint8_t databyte2 = 0;
volatile uint8_t Rhorizontal = 0;
volatile uint8_t Rvertical = 0;

int main(void)
{
    //main function initialization
    serial2_init();
    serial0_init();
    milliseconds_init();
    adc_init();
    lcd_init();
    uint32_t current_ms = 0;
    uint32_t last_send_ms = 0;
    while(1)
    {
        //main loop
        current_ms = milliseconds_now();
        
        Rhorizontal = adc_read(1)/5;
        Rvertical = adc_read(0)/5;
        
        databyte1 = Rhorizontal;
        databyte2 = Rvertical;
        //sending section
        if( (current_ms-last_send_ms) >= 100) //sending rate controlled here
        {
            //Function takes the number of bytes to send followed by the databytes as arguments
            serial2_write_bytes(2, databyte1, databyte2); 
            last_send_ms = current_ms;
        }
        _delay_ms(10);
    }
} 