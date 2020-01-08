#include "ws2812b.h"
#define DIGITALPINNUMBER  5
#define NUM_LEDS  16


ws2812b ledstrip = ws2812b(DIGITALPINNUMBER , NUM_LEDS);

void setup() {
  Serial.begin(9600);
  Serial.println("ws2812b test");
  ledstrip.begin();
  ledstrip.setPixelColor(5,0,255,0);
  ledstrip.setPixelColor(2,0,0,255);
  ledstrip.setPixelColor(8,255,0,0);
  ledstrip.show();
}


void loop() {
  
  delay(500);
}
