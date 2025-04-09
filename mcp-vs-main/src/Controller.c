//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Controller.h"
#include "C:\Users\jackg\Desktop\University\2.1\MXEN2003\mcp-vs-main\lib\adc\adc.h" //minimal adc lib

int main(void)
{
  serial0_init();
  adc_init(); //initialse ADC
  _delay_ms(20); //it's a good idea to wait a bit after your init section 

  uint16_t adcVal0 = 0;
  uint16_t adcVal1 = 0;

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

  DDRF = 0; //Ports for ADC
  PORTF = 0;

  DDRB = 0xFF; //Ports for PWM

  sei();

  while(1){
    adcVal0 = adc_read(0)+1000; //ADC ranges from 0-1023 so adding 1000 fits roughly within our 1000-2000 range of angles
    adcVal1 = adc_read(1)+1000;
    OCR1A = adcVal0;
    OCR1B = adcVal1;
    _delay_ms(10);
  }
} 