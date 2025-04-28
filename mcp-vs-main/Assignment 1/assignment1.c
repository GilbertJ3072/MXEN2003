#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util/delay.h>
#include <stdio.h>
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
2. Specific timer values important to operation
3. Error codes used for various errors that may occur during operation
4. Variables to be used during sensor operation
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
#define PING_SENSOR_PIN PD0                         //Pin 2 in Port D is where the ping sensor will be attached
#define PING_SENSOR_PORT PORTD                      //Associated port defined accordingly
#define PING_SENSOR_DDR DDRD                        //Associated data direction register defined accordingly
#define PING_SENSOR_DELAY_COUNT 200*16-1            // 200 µs (delay for next measurement)
#define PING_SENSOR_PULSE_COUNT 5*16-1              // 5 µs (pulse time)
#define PING_SENSOR_HOLDOFF_COUNT (750+100)*16-1    // 850 µs (echo holdoff time +100µs)
#define PING_SENSOR_NORESPONSE_CODE 65535           // The NO RESPONSE code
#define PING_SENSOR_ERROR_CODE 65534                // The ECHO PULSE ERROR code
#define CONVERTING_CONSTANT 0.000174                //Constant used to convert from time in µs to distance in mm

//Global variables for PING))) Sensor
volatile uint16_t pingMicros;                       //Defines variable for the elapsed micro seconds
volatile uint32_t pingValue;                        //Defines variable for the final value of ping sensor (used when measurement is taken)
volatile bool newReading;                           //Boolean variable used to keep track of new readings

/***********************************************************************
Short Description

Code executed when Timer1 Overflows which occurs every ~4.1ms
- Increments pingMicros 4ms
- Checks if more than 18.5ms has elapsed and if so:
- An error has occurred as maximum pulse detection time is 18.5ms. The error is printed to serial
************************************************************************/
ISR(TIMER1_OVF_vect)                                //Will trigger on the overflow of timer1 (4.1ms)
{
    if(!newReading)                                 //If no new readings have been taken at overflow
    {
        pingMicros += 4000;                         //Add 4ms to elapsed time 
        if(pingMicros > 18500)                      //If over 18.5ms have elapsed (Echo return pulse maximum time)
        {
            newReading = true;
            pingMicros = PING_SENSOR_ERROR_CODE;     //Raise echo pulse error flag
        }
    }
}

/***********************************************************************
Short Description

Interrupt associated with timings of the sensor
- After the 200µs delay between pulses, the ping sensor is set to output for 5µs.
- The ping sensor is then returned to input, INT0 is enabled on rising edge and a delay of 850µs is enabled
- If INT0 is not triggered within 850µs this indicated
************************************************************************/
ISR(TIMER1_COMPA_vect)                              //Interrupt executed when TCNT1 (timer count) = OCR1A (output compare register)
{
    if(OCR1A == PING_SENSOR_DELAY_COUNT)             //Delay for next measurement
    {
        PING_SENSOR_DDR |= (1<<PING_SENSOR_PIN);    //Sets ping sensor pin as an output pin
        PING_SENSOR_PORT |= (1<<PING_SENSOR_PIN);   //Enables pullup resistor
        OCR1A = PING_SENSOR_PULSE_COUNT;            //Sets OCR1A to 5µs
        TCNT1 = 0;                                  //Resets timer count
    }
    else if(OCR1A == PING_SENSOR_PULSE_COUNT)       //Delay for pulse time
    {
        PING_SENSOR_DDR &= ~(1<<PING_SENSOR_PIN);   //Sets ping sensor pin back to input pin
        PING_SENSOR_PORT &= ~(1<<PING_SENSOR_PIN);  //Disables pullup resistor
        OCR1A = PING_SENSOR_HOLDOFF_COUNT;          //Sets OCR1A TO 850µs
        TCNT1 = 0;                                  //Resets timer count
        //Initialising INT0 interrupt
        EICRA |= (1<<ISC01)|(1<<ISC00);             //Trigger on rising edge
        EIFR = (1<<INTF0);                          //Clear interrupt flags on INT0
        EIMSK |= (1<<INT0);                         //Enable interrupt 0
    }
    else if(OCR1A == PING_SENSOR_HOLDOFF_COUNT)     //If ping sensor has not started transmitting high after holdoff time:
    {
        pingMicros = PING_SENSOR_NORESPONSE_CODE;   //Raise no response error code
        newReading = true;
    }    
}

/***********************************************************************
Short Description

Interrupt associated with the measuring duration of the pulse:
- After holdoff time the sensor begins transmitting high, timer is reset, output compare is disabled and INT0 switched to falling edge
- When pulse is detected on falling edge the elapsed time of the pulse is recorded in pingValue and INT0 is disabled
************************************************************************/
ISR(INT0_vect)                                      //INT0 Pin (PD2) 
{
    if(PIND & (1<<PING_SENSOR_PIN))                 //If triggered on rising edge
    {
        TCNT1 = 0;                                  //Reset timer counter to 0
        pingMicros = 0;                             //Resets value ping micro seconds to 0
        TIMSK1 &= ~(1<<OCIE1A);                     //Turns off Output Compare Match
        EICRA &= ~(1<<ISC00);                       //Setting EICRA so that the INT0 triggers on falling edge
        EICRA |= (1<<ISC01);                        //cont.
    }else                                           //If triggered on falling edge    
    {
        pingValue = (pingMicros + (TCNT1>>4))*((STUDENT_ID/7)%1000);    //Sets pingValue to (total microseconds elapsed)*988
        newReading = true; 
        EIMSK &= ~(1<<INT0);                        //Disables INT0
    }
}

/***********************************************************************
Short Description

Initialises all functions involved

Executes printing to serial monitor depending on output from interrupts:
- If there is signal detected it will print this to the serial monitor
- If there is no pulse detected in maximum pulse time it prints error to serial
- If the pulse is successfully detected and is within range the distance is printed to serial
- If the pulse is detected but is not within range it prints object too far to serial
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
            if (pingMicros == PING_SENSOR_NORESPONSE_CODE)
            {
                serial0_print_string("No response\n");          //Occurs if sensor is not transmitting high after holdoff time
            }
            else if (pingMicros == PING_SENSOR_ERROR_CODE)
            {
                serial0_print_string("Echo Pulse Error\n");     //Occurs if 18.5ms have passed with no detections (max pulse detection time)
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

Defining function Ping Sensor Start (called after each measurement from the sensor):
- Sets ping sensor signal pin to input
- Enables output compare interrupts
- Sets OCR1A to the inter-reading delay value 
- Sets timer count to 0
************************************************************************/
void ping_sensor_start(void)
{
    PING_SENSOR_DDR &= ~(1<<PING_SENSOR_PIN);           //Ping sensor pin an input pin
    PING_SENSOR_PORT &= ~(1<<PING_SENSOR_PIN);          //Disables pullup resistor
    OCR1A = PING_SENSOR_DELAY_COUNT;                    //Sets output compare register 1A value to 200µs
    TIMSK1 |= (1<<OCIE1A);                              //Output Compare interrupt enabled
    TCNT1 = 0;                                          //Sets timer counter to 0
}