//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Robot.h"

volatile uint8_t databyte1 = 0;
volatile uint8_t databyte2 = 0;
volatile uint8_t databyte3 = 0;
volatile uint8_t RangeSensorLeftValue = 0;
volatile uint8_t RangeSensorRightValue = 0;
volatile uint8_t RangeSensorCentreValue = 0;

//main function initialization

int main(void){
  serial2_init();
  milliseconds_init();
  adc_init();
  uint32_t current_ms = 0;
  uint32_t last_send_ms = 0;
  uint8_t recievedData[2]; //recieved data array
  char serial_string[60] = {0}; // String used for printing to terminal

  DDRF = 0; //Ports for ADC
  PORTF = 0;
  DDRB = 0xFF; //Ports for PWM

  cli();
  TCCR1A = 0;
  TCCR1B = 0;

  TCCR1A |= (1 << COM1A1); //PWM clear on up, set on down
  TCCR1A |= (1 << COM1B1); //PWM clear on up, set on down

  TCCR1B |= (1 << CS11); //Prescaler of 8
  TCCR1B |= (1 << WGM13); //Mode 8, PWM ICR1 as TOP

  //TIMSK1 = (1 << ICIE1); //setting to mode for capture (ICR1 is vector) 
  ICR1 = 20000; //Top Value (makes period 20ms)
  OCR1A = 1000; //Compare Value (duty) for horizontal joystick
  OCR1B = 1000; //Compare Value (duty) for vertical joystick

  DDRB = 0xFF; //Ports for PWM
  sei();

  while(1)
  {
    //main loop
    current_ms = milliseconds_now();

    RangeSensorCentreValue = adc_read(0)/5;
    RangeSensorLeftValue = adc_read(1)/5;
    RangeSensorRightValue = adc_read(2)/5;
    databyte1 = RangeSensorCentreValue;
    databyte2 = RangeSensorLeftValue;
    databyte3 = RangeSensorRightValue;

    //sending section
    if( (current_ms-last_send_ms) >= 100) //sending rate controlled here
        {
            //Function takes the number of bytes to send followed by the databytes as arguments
            serial2_write_bytes(3, databyte1, databyte2, databyte3); 
            last_send_ms = current_ms;
        }

    if(serial2_available()) //Returns true if new data available on serial buffer
    {
      //Function takes the array to return data to and the number of bytes to be read.
      serial2_get_data(recievedData,2); 
            sprintf(serial_string,"\nData 1: %3u, Data2: %3u", recievedData[0],recievedData[1]); //Format string
            serial0_print_string(serial_string); //Print string to usb serial
    }

    OCR1A = recievedData[0]*5+1000;
    OCR1B = recievedData[1]*5+1000;
    _delay_ms(10);
  }
}