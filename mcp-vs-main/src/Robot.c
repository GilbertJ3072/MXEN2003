//Example ATmega2560 Project
//File: ATmega2560Project.c
//An example file for second year mechatronics project

//include this .c file's header file
#include "Robot.h"

// TUNING VALUES
#define lightBoundaryValue 80 // Detecting light in LightSeek mode
#define lightDiffBoundaryValue 5 // Boundary for "seeing" light in LightSeek mode

#define sideBoundaryValue 50 // Turning when nothing in front in Autonomous mode
#define frontBoundaryValue 20 // Turning when something in front in Autonomous mode

#define frequencyDetectValue 70 // Detecting light in Frequency detect

volatile static int16_t lm = 0;
volatile static int16_t rm = 0;

//main function initialization

int main(void){
  serial2_init(); //Serial for Xbee communication
  serial0_init(); //Initialising serial port for serial communication to computer
  milliseconds_init(); 
  adc_init();
  uint32_t current_ms = 0;
  uint32_t last_send_ms = 0;
  uint8_t recievedData[4]; //recieved data array
  uint8_t mode = 0;
  
  //Variables associated with frequency detection
  
  bool LightDetect = false; 
  bool prevAboveThreshold = false;
  uint32_t lastEdgeTime = 0;
  uint32_t last_send_ms_freq = 0;

  uint8_t frequency = 0;
  uint8_t FREQdecimal = 0;

  int32_t FREQsum = 0;
  uint16_t FREQarray[4];
  int32_t decimalFREQsum = 0;
  uint16_t decimalFREQarray[4];

  //Variables associated with light seeking

  bool LightLook = false;
  uint32_t last_check_ms = 0; //used for light seek and autonomous modes
  uint16_t LPRaverage[10] = {0};
  uint16_t RPRaverage[10] = {0};
  uint16_t LPRambient = 0;
  uint16_t RPRambient = 0;
  uint32_t LPRsum = 0;
  uint32_t RPRsum = 0;
  int16_t lightDifference = 0;

  //Hardware and interrupt initialisation

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

  DDRF = 0xFF; // Ports for input to H-Bridge
  DDRK = 0; // ADC Ports
  DDRB = 0xFF; //Ports for PWM

/***********************************************************************
Code for the servo Motor!!!
************************************************************************/
  DDRE |= (1 << PE3);

  // Clear Timer/Counter Control Registers
  TCCR3A = 0;
  TCCR3B = 0;

  TCCR3A |= (1 << COM3A1); //PWM clear on up, set on down
  TCCR3B |= (1 << CS31); //Prescaler of 8
  TCCR3B |= (1 << WGM33); //Mode 8, PWM ICR1 as TOP

  ICR3 = 20000; //Top Value (makes period 20ms)
  OCR3A = 1200; //Compare value for joystick

  sei();

  while(1)
  {
/***********************************************************************
Reading in Photoresistor Values, Range sensor values and Exbee Databytes
************************************************************************/
    current_ms = milliseconds_now();

    uint16_t leftPhotoresistor = adc_read(12)/5;
    uint16_t rightPhotoresistor = adc_read(13)/5;

    uint16_t rightSensor = adc_read(11)/5;
    uint16_t frontSensor = adc_read(10)/5;
    uint16_t leftSensor = adc_read(9)/5;
    // sprintf(serial_string, " \n| L=%ucm, F=%ucm, R=%ucm", leftSensor, frontSensor, rightSensor);
    // serial0_print_string(serial_string);

    uint8_t batteryLevel = adc_read(8)/5;

    if(serial2_available()) //Returns true if new data available on serial buffer
    {
    //Function takes the array to return data to and the number of bytes to be read.
        serial2_get_data(recievedData,4); 
    }

    mode = recievedData[3];
    mode = 2;

/***********************************************************************
Autonomous Mode
************************************************************************/
    if (mode == 0){
      OCR3A = 1200; //Compare value for joystick
      if( (current_ms-last_check_ms) >= 100) //sending rate controlled here
      {
      OCR1A = 16000;
      OCR1B = 16000;

      if (frontSensor > frontBoundaryValue){
        if (leftSensor < rightSensor){
          PORTF |= (1<<PF0); //Turn Right
          PORTF &= ~(1<<PF1);
          PORTF &= ~(1<<PF2);
          PORTF |= (1<<PF3);
        }
        if (leftSensor > rightSensor){
          PORTF &= ~(1<<PF0); //Turn Left
          PORTF |= (1<<PF1);
          PORTF |= (1<<PF2);
          PORTF &= ~(1<<PF3);
        }
      }
      else{
        if (sideBoundaryValue < rightSensor){
          PORTF |= (1<<PF0); //Turn Right
          PORTF &= ~(1<<PF1);
          PORTF &= ~(1<<PF2);
          PORTF |= (1<<PF3);
        }
        else if (leftSensor > sideBoundaryValue){
          PORTF &= ~(1<<PF0); //Turn Left
          PORTF |= (1<<PF1);
          PORTF |= (1<<PF2);
          PORTF &= ~(1<<PF3);
        }else{
          PORTF |= (1<<PF0); //Move Forward
          PORTF &= ~(1<<PF1);
          PORTF |= (1<<PF2);
          PORTF &= ~(1<<PF3);
        }
      }
      last_check_ms = current_ms;
      }
    }
/***********************************************************************
Manual Mode
************************************************************************/
    else if (mode == 1){
      uint16_t fc = recievedData[1];
      uint16_t rc = recievedData[0];
      uint16_t joystickY = recievedData[2];
      OCR3A = 1200 + ((uint32_t)joystickY * 400) / 205;

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
    }
/***********************************************************************
Light Seek Mode
************************************************************************/
    else if (mode == 2){ // Light Seek Mode
      OCR3A = 1600;

      for (int i=9; i>=1; i--){
        LPRaverage[i]=LPRaverage[i-1];
        RPRaverage[i]=RPRaverage[i-1];
      }
      LPRaverage[0]=leftPhotoresistor;
      RPRaverage[0]=rightPhotoresistor;
      LPRsum = 0;
      RPRsum = 0;
      for (int i=0; i<=9; i++){
        LPRsum += LPRaverage[i];
        RPRsum += RPRaverage[i]; 
      }
      LPRambient = LPRsum/10;
      RPRambient = RPRsum/10;

      lightDifference = (int16_t)LPRambient - (int16_t)RPRambient;
      if (LPRambient < 80 || RPRambient < 80){
        LightLook = true;
      }
      OCR1A = 16000; 
      OCR1B = 16000; 
      if (LightLook == true){
        if (-10<lightDifference && 10>lightDifference){
          PORTF |= (1<<PF0); //Move Forward
          PORTF &= ~(1<<PF1);
          PORTF |= (1<<PF2);
          PORTF &= ~(1<<PF3);
        }else if (lightDifference > 10){
          PORTF &= ~(1<<PF0); //Turn Right
          PORTF |= (1<<PF1);
          PORTF |= (1<<PF2);
          PORTF &= ~(1<<PF3); 
        }else if (lightDifference < -10){
          PORTF |= (1<<PF0); //Turn Left
          PORTF &= ~(1<<PF1);
          PORTF &= ~(1<<PF2);
          PORTF |= (1<<PF3);
        }
      }
      if (LightLook == false){
          PORTF |= (1<<PF0); //Turn Left
          PORTF &= ~(1<<PF1);
          PORTF &= ~(1<<PF2);
          PORTF |= (1<<PF3); 
      }
      LightLook = false;
      last_check_ms = current_ms;
      
      
    }
/***********************************************************************
Beacon Frequency Detection
************************************************************************/
    if (current_ms - last_send_ms_freq >= 5) {

      last_send_ms_freq = current_ms;
      LightDetect = leftPhotoresistor < frequencyDetectValue;

      if (LightDetect && !prevAboveThreshold) {
          uint32_t currentTime = milliseconds_now();
          uint32_t period = currentTime - lastEdgeTime;
          lastEdgeTime = currentTime;

          if (period > 0) {
            for (int i=3; i>=1; i--){
              FREQarray[i]=FREQarray[i-1];
              decimalFREQarray[i]=decimalFREQarray[i-1];
            }
            FREQarray[0]= (1000) / period;
            decimalFREQarray[0] = (10000 + period - 1) / (period) - (1000/period)*10;
            FREQsum = 0;
            decimalFREQsum = 0;
            for (int i=0; i<=3; i++){
              FREQsum += FREQarray[i];
              decimalFREQsum += decimalFREQarray[i];
            }
            frequency = FREQsum/4;
            FREQdecimal = (decimalFREQsum + 3 )/4;
            if (frequency > 40){
              frequency = 40;
            }
            if (frequency < 0){
              frequency = 0;
            }
          }
        }
      }
    if (LightDetect && leftPhotoresistor < (frequencyDetectValue - 2)) {
      prevAboveThreshold = true;
    } else if (!LightDetect && leftPhotoresistor > (frequencyDetectValue + 2)) {
      prevAboveThreshold = false;
    }
/***********************************************************************
XBee sending data
************************************************************************/
    if( (current_ms-last_send_ms) >= 100) //sending rate controlled here
    {
        //Function takes the number of bytes to send followed by the databytes as arguments
        serial2_write_bytes(6, leftPhotoresistor, rightSensor, frontSensor, leftSensor, frequency, FREQdecimal); 
        last_send_ms = current_ms;
    }

    if(batteryLevel<120){//monitoring battery level, LED will come on when battery level drops below level
      PORTF |= (1<<PF4);
    }else{
      PORTF &= ~(1<<PF4);
    }

    char serial_string[60] = {0}; // String used for printing to terminal
    //sprintf(serial_string, "\nFreq = %u", frequency);
    //serial0_print_string(serial_string);
    //sprintf(serial_string, ".%uHz", FREQdecimal);
    //serial0_print_string(serial_string);
    
    //sprintf(serial_string, "\nlpr = %u, rpr = %u", leftPhotoresistor, rightPhotoresistor);
    //serial0_print_string(serial_string);

    sprintf(serial_string, "\nAVlpr = %u, AVrpr = %u, diff= %d, lpr = %u, rpr = %u", LPRambient, RPRambient, lightDifference, leftPhotoresistor, rightPhotoresistor);
    //sprintf(serial_string, "\nBatt=%u", batteryLevel);
    serial0_print_string(serial_string);

    

    _delay_ms(1);
  }
}