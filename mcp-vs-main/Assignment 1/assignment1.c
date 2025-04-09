#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util/delay.h>
#include <stdio.h>

/************************************************************************
Serial functions included for simulation on TinkerCad 
************************************************************************/
//Define USART constants for ATmega328, see ATmega328P datasheet, pg 145
#define USART_BAUDRATE 9600
#define F_CPU 16000000
#define BAUD_PRESCALE ((((F_CPU/16)+(USART_BAUDRATE/2))/(USART_BAUDRATE))-1)

/************************************************************************
Initialise USART 0
See ATmega328P datasheet for register descriptions, pg 159
Input: None
Output: None
************************************************************************/
void serial0_init(void)
{
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);     //Enable bits for transmit and recieve
	UCSR0C = (1<<UCSZ00)|(1<<UCSZ01);   //Use 8- bit character sizes
	UBRR0 = BAUD_PRESCALE;              //Load baud rate prescaler into register
}

/************************************************************************
Print string via USART 0
See ATmega328P datasheet for register descriptions, pg 159
Input:      string_pointer      char array      string to be printed to serial 0
Output:     None
************************************************************************/
void serial0_print_string(char * string_pointer) 
{
	while(*string_pointer)              //While not null character (end of string)
	{
		while((UCSR0A&(1<<UDRE0))==0){} //Wait for register empty flag
		UDR0 = *string_pointer;         //Send what's at the string pointer to serial data register
		string_pointer++;               //Increment string pointer to go to next letter in string
	}
}

//********************************************************************//
//*************************Comment Below Here*************************//
//********************************************************************//

#define STUDENT_ID 21468919

/***********************************************************************
Short Description

Initialising the timer:
- Clear timer on compare mode
- Top value and prescaler set so timer resets approximately every 4.1ms

Initialising the sensor pins:
- Defining many variables relating to the PING)) sensor including:
1. Ports, Pins and DDR associated with the sensor
2. Specific timer values important to operation (x16 is for the 16MHz clock on the micro-controller)
3. Variables to be used during sensor operation
************************************************************************/
void ping_timer_init(void)
{
	cli();                                          //Disable global interrupts
    TCCR1A = 0;            
	TCCR1B = (1<<WGM12)|(1<<WGM13);                 //Sets to mode 12 (CTC with ICR1 as TOP)
	TCNT1 = 0;                                      //Reset timer counter to 0                   
	ICR1 = 65535;                                   //Set TOP value to 65535         
	TIMSK1 |= (1<<TOIE1);                           //Overflow interrupt enable   
	TCCR1B |= (1<<CS10);                            //Sets Prescaler to 1 (prescaler off)        
	sei();                                          //Enable global interrupts                   
}

//PING))) Sensor Pins
#define PING_SENSOR_PIN PD2                         //Pin 2 in Port D is where the ping sensor will be attached
#define PING_SENSOR_PORT PORTD                      //Associated port defined accordingly
#define PING_SENSOR_DDR DDRD                        //Associated data direction register defined accordingly
#define PING_SENSOR_$$$1_COUNT 200*16-1             // 200 µs (delay for next measurement)
#define PING_SENSOR_$$$2_COUNT 5*16-1               // 5 µs (trigger pulse time)
#define PING_SENSOR_$$$3_COUNT (750+100)*16-1       // 850 µs (echo holdoff time +100µs)
#define PING_SENSOR_$$$4_CODE 65535                 // The NO RESPONSE code
#define PING_SENSOR_$$$5_CODE 65534                 // The ERROR code (for overflow)
#define CONVERTING_CONSTANT 1                       //Calculate value for this!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//Global variables for PING))) Sensor
volatile uint16_t pingMicros;                       //Defines variable for the elapsed micro seconds
volatile uint32_t pingValue;                        //Defines variable for the value of ping sensor
volatile bool newReading;                           //Boolean variable used to keep track of new readings

/***********************************************************************
Short Description

Code executed when Timer1 Overflows which occurs every ~4.1ms
- Increments pingMicros by 4000 (adding the 4ms)
- Checks if more than 18.5ms has elapsed and if so:
- New reading is true and ping Micros is set to 0xFFFE (1 below max value)
************************************************************************/
ISR(TIMER1_OVF_vect)                                //Will trigger on the overflow of timer1 (4.1ms)
{
    if(!newReading)                                 //No new readings
    {
        pingMicros += 4000;                         //Add 4ms to elapsed time 
        if(pingMicros > 18500)                      //If over 18.5ms have elapsed (Echo return pulse maximum time)
        {
            newReading = true;
            pingMicros = PING_SENSOR_$$$5_CODE;     //Sets value of ping micros to 0xFFFE (error code)
        }
    }
}

/***********************************************************************
Short Description

************************************************************************/
ISR(TIMER1_COMPA_vect)                              //Interrupt executed when TCNT1 (timer count) = OCR1A (output compare register)
{
    if(OCR1A == PING_SENSOR_$$$1_COUNT)             //Delay for next measurement
    {
        PING_SENSOR_DDR |= (1<<PING_SENSOR_PIN);    //Sets ping sensor pin as an input pin
        PING_SENSOR_PORT |= (1<<PING_SENSOR_PIN);   //Enables pullup resistor
        OCR1A = PING_SENSOR_$$$2_COUNT;             //Sets OCR1A to 5µs
        TCNT1 = 0;                                  //Resets timer count
    }
    else if(OCR1A == PING_SENSOR_$$$2_COUNT)        //Delay for pulse time in micro-controller
    {
        PING_SENSOR_DDR &= ~(1<<PING_SENSOR_PIN);   //Sets ping sensor pin back to output pin
        PING_SENSOR_PORT &= ~(1<<PING_SENSOR_PIN);  //Disables pullup resistor
        OCR1A = PING_SENSOR_$$$3_COUNT;             //Sets OCR1A TO 850µs
        TCNT1 = 0;                                  //Resets timer count
        //Initialising INT0 interrupt
        EICRA |= (1<<ISC01)|(1<<ISC00);             //Trigger on rising edge
        EIFR = (1<<INTF0);                          //Clear interrupt flags on INT0
        EIMSK |= (1<<INT0);                         //Enable interrupt 0
    }
    else if(OCR1A == PING_SENSOR_$$$3_COUNT)        //Echo holdoff time
    {
        pingMicros = PING_SENSOR_$$$4_CODE;         //Set to no response code
        newReading = true;
    }    
}

/***********************************************************************
Short Description
************************************************************************/
ISR(INT0_vect)                                      //Triggered when INT0 Pin (PD2) 
{
    if(PIND & (1<<PING_SENSOR_PIN))                 //If PING sensor is set to input
    {
        TCNT1 = 0;                                  //Reset timer counter to 0
        pingMicros = 0;                             //Resets value ping micro seconds to 0
        TIMSK1 &= ~(1<<OCIE1A);                     //Turns off Output Compare Match
        EICRA &= ~(1<<ISC00);                       //Setting EICRA so that the INT0 triggers on falling edge
        EICRA |= (1<<ISC01);                        //cont.
    }else                                           //READING BEING TAKEN    
    {
        pingValue = (pingMicros + (TCNT1>>4))*((STUDENT_ID/7)%1000);    //Sets pingValue to (total microseconds elapsed)*988
        newReading = true; 
        EIMSK &= ~(1<<INT0);                        //Disables INT0
    }
}

/***********************************************************************
Short Description

Initialises all functions involved
************************************************************************/
int main(void)
{
    serial0_init();                                 //Initialises output to the serial monitor
    ping_timer_init();                              //Initialises the ping timer
    ping_sensor_start();                            //Initialises the ping sensor function
    char serial_string[16] = {0};                   //Initialises a string for serial printing
    uint16_t distance = 0;                          //Creates variable "distance"

    while (1)                                       //Infinite loop (main program)
    {   
        if(newReading)                              //If new reading is true
        {
            if (pingMicros == PING_SENSOR_$$$4_CODE)
            {
                serial0_print_string("No response\n"); //Occurs if INT0 is not triggered in time
            }
            else if (pingMicros == PING_SENSOR_$$$5_CODE)
            {
                serial0_print_string("Echo Pulse Error\n"); //Occurs if 18.5ms have passed with overflow occurring
            }
            else
            {
                distance = CONVERTING_CONSTANT*pingValue;   //Converts the elapsed time to distance (in mm) using the converting constant
                if(distance < 3000)
                {
                    sprintf(serial_string,"%4u mm\n",distance); //Printing the distance in mm to the serial port
                    serial0_print_string(serial_string);        //Executing the print
                }
                else
                {
                    serial0_print_string("Object too far\n");   //Outside of linear range
                }
            }
            ping_sensor_start();                                //Restarts the ping sensor
            newReading = false;                                 //Resets newReading boolean 
        }
    }
}

/***********************************************************************
Short Description

Defining function Ping Sensor Start:
- Allows reading of ping sensor
- Enables output compare match interrupts
- Sets timer count to 0
************************************************************************/
void ping_sensor_start(void)
{
    PING_SENSOR_DDR &= ~(1<<PING_SENSOR_PIN);           //Ping sensor pin an input pin
    PING_SENSOR_PORT &= ~(1<<PING_SENSOR_PIN);          //Disables pullup resistor
    OCR1A = PING_SENSOR_$$$1_COUNT;                     //Sets output compare register 1A value to 200µs
    TIMSK1 |= (1<<OCIE1A);                              //Output Compare interrupt enabled
    TCNT1 = 0;                                          //Sets timer counter to 0
}