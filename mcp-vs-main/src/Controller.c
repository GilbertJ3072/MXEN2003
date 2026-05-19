//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Controller.h"
#include "C:\Users\jackg\Desktop\University\2.1\MXEN2003\mcp-vs-main\lib\adc\adc.h" //minimal adc lib

volatile uint8_t Rhorizontal = 0;
volatile uint8_t Rvertical = 0;
#define debouncePeriod 200
volatile bool buttonPressed = false;

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
    uint8_t mode = 0;
    uint8_t recievedData[6]; //recieved data array
    char serial_string[60] = {0}; // String used for printing to terminal
/***********************************************************************
Port setup for left joystick button
************************************************************************/
    DDRD &= ~(1 << PD1);    // Clear PD1 direction (input)
    PORTD |= (1 << PD1);    // Enable pull-up resistor

    // Configure INT1 to trigger on falling edge
    EICRA |= (1 << ISC11);   // ISC11 = 1, ISC10 = 0 => Falling edge
    EICRA &= ~(1 << ISC10);

    // Enable INT1
    EIMSK |= (1 << INT1);
    uint8_t leftPhotoresistor = 0;
    uint8_t rightSensor = 0;
    uint8_t frontSensor = 0;
    uint8_t leftSensor = 0;
    uint8_t frequency = 0;
    uint8_t FREQdecimal = 0;

    while(1)
    {
        //main loop
        current_ms = milliseconds_now();
/***********************************************************************
XBee communication
************************************************************************/
        Rhorizontal = 204-adc_read(1)/5; //Right joystick values for manual mode movements
        Rvertical = 204-adc_read(0)/5;
        uint8_t Lvertical = adc_read(15)/5; //Left joystick vertical for claw movements
        
        if (buttonPressed) {
            buttonPressed = false;
            mode = (mode + 1) % 3; //Mode cycle (0=Autonomous, 1=Manual, 2=Light seeking)
        }

        //sending section
        if( (current_ms-last_send_ms) >= 100) //sending rate controlled here
        {
            //Function takes the number of bytes to send followed by the databytes as arguments
            serial2_write_bytes(4, Rhorizontal, Rvertical, Lvertical, mode); 
            last_send_ms = current_ms;
        }

        if(serial2_available()) //Returns true if new data available on serial buffer
        {
        //Function takes the array to return data to and the number of bytes to be read.
            serial2_get_data(recievedData,6); 
            leftPhotoresistor = recievedData[0];
            rightSensor = recievedData[1];
            frontSensor = recievedData[2];
            leftSensor = recievedData[3];
            frequency = recievedData[4];
            FREQdecimal = recievedData[5];
        }
// Mode ----------------------------------------------------
        if (mode == 0){
            lcd_clrscr();
            lcd_goto(0x40);
            lcd_puts("Autonomous Mode");
        }
        if (mode == 1){
            lcd_clrscr();
            lcd_goto(0x40);
            lcd_puts("Manual Mode");
        }
        if (mode == 2){
            lcd_clrscr();
            lcd_goto(0x40);
            lcd_puts("Beacon Seek Mode");
        }
// Serial Printing ----------------------------------------------------


        if (leftPhotoresistor < 80){
            lcd_goto(0x0);
            lcd_puts("Beacon Detect");
        }
        else{
            lcd_goto(0x0);
            lcd_puts("No Beacon Detect");
        }

        uint16_t rightDist = (12*204.0)/((rightSensor+1)*5); 
        uint16_t frontDist = (12*204.0)/((frontSensor+1)*5); 
        uint16_t leftDist = (12*204.0)/((leftSensor+1)*5);
        if (leftDist > 45){
            leftDist = 45;
        }if (rightDist > 45){
            rightDist = 45;
        }if (frontDist > 45){
            frontDist = 45; 
        }

        sprintf(serial_string, "\nFreq = %u", frequency);
        serial0_print_string(serial_string);
        sprintf(serial_string, ".%uHz", FREQdecimal);
        serial0_print_string(serial_string);
        sprintf(serial_string, " | L=%ucm, F=%ucm, R=%ucm", leftDist, frontDist, rightDist);
        serial0_print_string(serial_string);

        _delay_ms(1);
    }
}

ISR(INT1_vect) {
    uint32_t Time = milliseconds_now();
    static uint32_t previousTime = 0;
    if ((Time - previousTime) > debouncePeriod){
        previousTime = Time;
        buttonPressed = true;
    }
}
