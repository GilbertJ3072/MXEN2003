//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Controller.h"
#include "C:\Users\jackg\Desktop\University\2.1\MXEN2003\mcp-vs-main\lib\adc\adc.h" //minimal adc lib

char serialString[20] = {};
volatile uint16_t MScount = 0;
volatile uint16_t Scount = 0;
volatile uint16_t MINcount = 0;
volatile uint16_t toggler = 0;
volatile uint16_t reset = 0;
volatile 
int main(void)
{
  milliseconds_init();
  serial0_init();
  adc_init(); //initialse ADC
  _delay_ms(20); //it's a good idea to wait a bit after your init section
  adc_read(0);

  cli();
  TCCR1A = 0;
  TCCR1B = 0b00011001; //A and B setting mode (mode 12) and prescaler (1)
  TIMSK1 = (1 << ICIE1); //setting to mode for capture (ICR1 is vector) 
  ICR1 = 15999; //made so that period of timer is 1ms (calculations using formula in book)

  DDRD = 0;
  PORTD = 0;

  EICRA |= (1<<ISC01) | (1<<ISC00);
  EICRA |= (1<<ISC11) | (1<<ISC10);
  EIMSK = (1<<INT0) | (1<<INT1);

  sei();

  sprintf(serialString, "TIMER");
  serial0_print_string(serialString);

  while(1){
    sprintf(serialString, "%umin   %usec   %ums\n",MINcount, Scount, MScount);
    serial0_print_string(serialString);
    _delay_ms(20);
  }
}

ISR(TIMER1_CAPT_vect){ 
  MScount += 1;
  if (MScount == 1000){
    MScount = 0;
    Scount += 1;
  }
  if (Scount == 60){
    Scount = 0;
    MINcount += 1;
  }
}

ISR(INT1_vect)
{
  uint32_t Time = milliseconds_now();
  static uint32_t previousTime = 0;
  if ((Time - previousTime) > 100){
    TIMSK1 ^= (1 << ICIE1);
    previousTime = Time;
  }
}

ISR(INT0_vect)
{
  uint32_t Time = milliseconds_now();
  static uint32_t previousTime = 0;
  if ((Time - previousTime) > 100){
    MScount = 0;
    Scount = 0;
    MINcount = 0;
    previousTime = Time;
  }
}