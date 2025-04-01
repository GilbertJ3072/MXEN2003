//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Controller.h"
#include "C:\Users\jackg\Desktop\University\2.1\MXEN2003\mcp-vs-main\lib\adc\adc.h" //minimal adc lib

#define debouncePeriod 100
volatile uint16_t temp = 0;
volatile uint16_t numEdges = 0;
volatile uint16_t adcVal = 0;
char serialString[10] = {};

int main(void)
{
  milliseconds_init();
  serial0_init(); // initialise the serial comms
  adc_init(); //initialse ADC
  _delay_ms(20); //it's a good idea to wait a bit after your init section
  adc_read(0); //read the voltage at PF0 (horizontal position)
  
  DDRF = 0; // set port F (ADC) to read

  cli(); // initialising the interrupts (turn off for now)
  EICRA |= (1<<ISC01);
  EICRA |= (1<<ISC00); // 11 makes it trigger on rising edge
  EIMSK |= (1<<INT0); //setting pin
  DDRD = 0;
  PORTD = 0xFF;
  sei();

  while(1)
  {
    uint32_t currentTime = milliseconds_now();
    if (currentTime % 1000 == 0)
    {
      sprintf(serialString, "Edge = %u\n", numEdges);
      serial0_print_string(serialString);
      numEdges = 0;
    }
  }
  return(1);
}

ISR(INT0_vect)
{
  uint32_t Time = milliseconds_now();
  static uint32_t previousTime = 0;
  if ((Time - previousTime) > debouncePeriod){
    numEdges += 1;
    previousTime = Time;
  }
}   