//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Controller.h"
#include "C:\Users\jackg\Desktop\University\2.1\MXEN2003\mcp-vs-main\lib\adc\adc.h" //minimal adc lib

volatile uint8_t databyte1 = 0;
volatile uint8_t databyte2 = 0;
volatile uint8_t Lvertical = 0;
volatile uint8_t Rvertical = 0;

volatile uint16_t distance = 0;
volatile uint16_t voltage = 0;

volatile uint16_t RangeSensorLeftValue = 0;
volatile uint16_t RangeSensorRightValue = 0;
volatile uint16_t RangeSensorCentreValue = 0;

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
    uint8_t recievedData[3]; //recieved data array
    char serial_string[60] = {0}; // String used for printing to terminal
    while(1)
    {
        //main loop
        current_ms = milliseconds_now();
        
        Lvertical = adc_read(15)/5;
        Rvertical = adc_read(0)/5;
        
        databyte1 = Lvertical;
        databyte2 = Rvertical;
        //sending section
        if( (current_ms-last_send_ms) >= 100) //sending rate controlled here
        {
            //Function takes the number of bytes to send followed by the databytes as arguments
            serial2_write_bytes(2, databyte1, databyte2); 
            last_send_ms = current_ms;
        }
        if(serial2_available()) //Returns true if new data available on serial buffer
        {
            //Function takes the array to return data to and the number of bytes to be read.
            serial2_get_data(recievedData,3); 
        }

        //LCD SHIT

        RangeSensorCentreValue = recievedData[0];
        RangeSensorLeftValue = recievedData[1];
        RangeSensorRightValue = recievedData[2];

        lcd_clrscr();
        sprintf(serial_string, "%u %u %u", RangeSensorCentreValue, RangeSensorLeftValue, RangeSensorRightValue);
        lcd_goto(0x40);
        lcd_puts(serial_string);
        if (RangeSensorCentreValue > 80){
            lcd_goto(0);
            lcd_puts("Stationary");
        }
        else if (RangeSensorLeftValue > 80){
            lcd_goto(0);
            lcd_puts("Turn Right");
        }
        else if (RangeSensorRightValue > 80){
            lcd_goto(0);
            lcd_puts("Turn Left");
        }
        else{
            lcd_goto(0);
            lcd_puts("Continue Forward");
        }

        _delay_ms(10);
    }
} 