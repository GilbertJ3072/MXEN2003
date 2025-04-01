//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Controller.h"
#include "C:\Users\jackg\Desktop\University\2.1\MXEN2003\mcp-vs-main\lib\adc\adc.h" //minimal adc lib

int main(void)
{
  adc_init(); //initialse ADC
  _delay_ms(20); //it's a good idea to wait a bit after your init section
  adc_read(0); //read the voltage at PF0 (horizontal position)
  
  DDRF = 0;
  
  DDRC = 0xFF;
  PORTC = 0;

  uint16_t adcVal = 0;
  uint8_t lightPin = 0;

  while(1)
  {
    adcVal = adc_read(0);
    lightPin = adcVal/128;
    _delay_ms(20);
    PORTC = (1<<lightPin); 
  }
  return(1);
}