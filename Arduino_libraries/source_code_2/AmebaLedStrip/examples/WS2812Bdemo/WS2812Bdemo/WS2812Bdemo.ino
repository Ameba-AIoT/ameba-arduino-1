/*
  This sketch shows how to use setPixelColor(element, Red, Green, Blud) 
  to light up a  element in WS2812B LED matrix.

*/
#include "ws2812b.h"
#define DIGITALPINNUMBER  5
#define NUM_LEDS  16


ws2812b ledstrip = ws2812b(DIGITALPINNUMBER , NUM_LEDS);

void setup() {
  Serial.begin(9600);
  Serial.println("ws2812b LED strip demo");
  ledstrip.begin();
  ledstrip.setPixelColor(5,0,10,0);
  ledstrip.setPixelColor(2,0,0,10);
  ledstrip.setPixelColor(1,10,10,10);
  ledstrip.setPixelColor(3,10,0,0);
  ledstrip.setPixelColor(8,10,0,0);

  ledstrip.show();
}


void loop() {
  
  delay(500);
}
