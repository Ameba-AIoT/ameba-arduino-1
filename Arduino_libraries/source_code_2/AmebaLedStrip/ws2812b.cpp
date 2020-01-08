
#include "ws2812b.h"
#include "rtl8195a.h"
#include "FreeRTOS.h"
#include "task.h"
#if defined(__AVR__)
#include <util/delay.h>
#endif

ws2812b::ws2812b(int input_pin, int num_leds) {
	_input_pin = input_pin;
	_num_leds = num_leds;
}


void ws2812b::begin(void) {
 	
  pinMode(_input_pin, OUTPUT);
  mask = digitalPinToBitMask(_input_pin);
  port = digitalPinToPort(_input_pin);
  
  ledBGColor =(int **) malloc(_num_leds *sizeof(int*));
		
	for(int i = 0; i < _num_leds; i++)
	{
	    ledBGColor[i] = (int *)malloc(NUM_COLORS*sizeof(int));
	    ledBGColor[i][0] = 0;
	    ledBGColor[i][1] = 0;
	    ledBGColor[i][2] = 0;	   
	}
  
}

void ws2812b::show(void) {
   sendLEDs();
}

void ws2812b::setPixelColor(int whichLedNumber, int rColor ,int gColor ,int bColor) {
		
		ledBGColor[whichLedNumber][0] = gColor;
		ledBGColor[whichLedNumber][1] = rColor;
		ledBGColor[whichLedNumber][2] = bColor;	
}

void ws2812b::sendByte(uint8_t data){
	
	uint8_t bits = 8;

    do {
        if ( (data & 0x80) == 0) { 
        	
        	for(int i = 0; i < CIRCLE_TIME; i++)
					{
            *portOutputRegister(port) = mask;
            *portOutputRegister(port) = mask;
					}

					for(int i = 0; i < CIRCLE_TIME; i++)
					{
            *portOutputRegister(port) = 0x00000000;
            *portOutputRegister(port) = 0x00000000;
            *portOutputRegister(port) = 0x00000000;
            *portOutputRegister(port) = 0x00000000;
            *portOutputRegister(port) = 0x00000000;
          }
				} else {
        	for(int i = 0; i < CIRCLE_TIME; i++)
					{	
            *portOutputRegister(port) = mask;
           	*portOutputRegister(port) = mask;
            *portOutputRegister(port) = mask;
           	*portOutputRegister(port) = mask;
          }
        	for(int i = 0; i < CIRCLE_TIME; i++)
					{          
            *portOutputRegister(port) = 0x00000000;
            *portOutputRegister(port) = 0x00000000;
            *portOutputRegister(port) = 0x00000000;
          }
      	}
        data += data;
    } while (-- bits != 0);	
	
}

void ws2812b::sendLEDs() {
    uint8_t i;
    taskENTER_CRITICAL(); 
    for (i = 0; i < _num_leds; ++ i) {      
      sendByte(ledBGColor[i][0]);
			sendByte(ledBGColor[i][1]);
      sendByte(ledBGColor[i][2]);
    }
    taskEXIT_CRITICAL(); 
}
