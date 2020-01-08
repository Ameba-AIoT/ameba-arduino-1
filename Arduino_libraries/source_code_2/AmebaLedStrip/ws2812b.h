#include "Arduino.h"
#include "gpio_api.h"

#if defined(BOARD_RTL8195A)
#define CIRCLE_TIME 2
#elif defined(BOARD_RTL8710)
#define CIRCLE_TIME 1
#else
#endif

#define NUM_COLORS	3

class ws2812b {
 public:
  ws2812b(int input_pin, int num_leds);
  void begin(void);
  void show(void);
  void sendByte(uint8_t b);
  void setPixelColor(int whichLedNumber, int rColor ,int gColor ,int bColor);  
  void sendLEDs();

 private: 	 	
 	int _num_leds;
 	int **ledBGColor;
 	int _input_pin;	
	uint32_t mask,port;
};

