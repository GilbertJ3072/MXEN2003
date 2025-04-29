//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Robot.h"

volatile uint8_t databyte1 = 0;
volatile uint8_t databyte2 = 0;
volatile static int16_t lm = 0;
volatile static int16_t rm = 0;


//main function initialization

int main(void){
  serial2_init();
  milliseconds_init();
  adc_init();
  uint32_t current_ms = 0;
  uint32_t last_send_ms = 0;
  uint8_t recievedData[2]; //recieved data array
  char serial_string[60] = {0}; // String used for printing to terminal

  DDRB = 0xFF; //Ports for PWM


  cli();
  TCCR1A = 0;
  TCCR1B = 0;

  TCCR1A |= (1 << COM1A1); //PWM clear on up, set on down
  TCCR1A |= (1 << COM1B1); //PWM clear on up, set on down

  TCCR1B |= (1 << CS10); //Prescaler of 1
  TCCR1B |= (1 << WGM13); //Mode 8, PWM ICR1 as TOP

  //TIMSK1 = (1 << ICIE1); //setting to mode for capture (ICR1 is vector) 
  ICR1 = 16000; //Top Value (makes period 20ms)
  OCR1A = 1000; //Compare Value (duty) for left joystick
  OCR1B = 1000; //Compare Value (duty) for right joystick

  DDRF = 0xFF;

  sei();

  while(1)
  {
    //main loop
    current_ms = milliseconds_now();

    if(serial2_available()) //Returns true if new data available on serial buffer
    {
      //Function takes the array to return data to and the number of bytes to be read.
      serial2_get_data(recievedData,2); 
            sprintf(serial_string,"\nData 1: %3u, Data2: %3u", recievedData[0],recievedData[1]); //Format string
            serial0_print_string(serial_string); //Print string to usb serial
    }

    uint16_t rc = recievedData[1];
    uint16_t fc = recievedData[0];

    lm = fc + rc - 204;
    rm = fc - rc;

    OCR1A = (int32_t)abs(lm)*16000/102; //lm speed from magnitude of lm
    OCR1B = (int32_t)abs(rm)*16000/102; //lm speed from magnitude of rm

    if(lm>=0) //if lm is positive
    {
      //set direction forwards
      PORTF |= (1<<PF0);
      PORTF &= ~(1<<PF1);
    }
    else
    {
      //set direction reverse
      PORTF &= ~(1<<PF0);
      PORTF |= (1<<PF1);
    }

    if(rm>=0) //if rm is positive
    {
      //set direction forwards
      PORTF |= (1<<PF2);
      PORTF &= ~(1<<PF3);
    }
    else
    {
      //set direction reverse
      PORTF &= ~(1<<PF2);
      PORTF |= (1<<PF3);
    }
    _delay_ms(10);
  }
}