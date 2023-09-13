#include "Particle.h"
#include "TopLevel_OLED.h"

#include "oled-wing-adafruit.h"

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char logo16_glcd_bmp[] =
{   B00000000, B11000000,
		B00000001, B11000000,
		B00000001, B11000000,
		B00000011, B11100000,
		B11110011, B11100000,
		B11111110, B11111000,
		B01111110, B11111111,
		B00110011, B10011111,
		B00011111, B11111100,
		B00001101, B01110000,
		B00011011, B10100000,
		B00111111, B11100000,
		B00111111, B11110000,
		B01111100, B11110000,
		B01110000, B01110000,
		B00000000, B00110000 };

OledWingAdafruit display;




bool line1State = false;
bool line2State = false;
bool line3State = false;

void OLED_setup() {
	display.setup();

	display.clearDisplay();
	display.display();
    display.setTextSize(1); // 1: 6x8
	display.setTextColor(WHITE);
    display.setCursor(0,0);
	display.println("DISP INIT");
    display.display();
}

void OLED_CheckButtons(bool &A, bool &B, bool &C) {
	display.loop(); // loop calls Debounce::update() which captures button press state
	A = display.pressedA();
	B = display.pressedB();
	C = display.pressedC();

	if(display.pressedB()){
		Serial.println("Button B was Pressed");
	}

	/*if (display.pressedA()) {
		display.clearDisplay();
        display.setTextSize(1);
	    display.setTextColor(WHITE);
		display.setCursor(0,0);
        if(line1State){
            line1State = false;
            display.println("GS MEAS STARTED");
        }
        else{
            line1State = true;
            display.println("GS MEAS STOPPED");
        }
		display.display();
	}
	if (display.pressedB()) {
		display.setCursor(0,9);
               if(line2State){
            line2State = false;
            display.println("LOGGING TO SD CARD");
        }
        else{
            line2State = true;
            display.println("SD CARD LOGGING STOP ");
        }
        display.display();
		//graphicsTest();
	}

	if (display.pressedC()) {
		display.setCursor(0,18);
        if(line3State){
            line3State = false;
        }
        else{
            line3State = true;
        }
        display.println("PUMP RUNNING");
        display.display();
		//bitmapTest();
	}*/
	return;
}

void OLED_UpdateDisplay(char* line1, char* line2, char* line3){
    // Display: 128 x 32
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(line1);
    display.setCursor(0,10);
    display.println(line2);
    display.setCursor(0,20);
    display.println(line3);
    display.display();
}

void OLED_OneLongString(char* dispData){
    // Display: 128 x 32
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(dispData);
    display.display();
}

void OLED_graphicsTest() {
	display.clearDisplay();
	{
		for (int16_t i=0; i<display.height()/2-2; i+=2) {
			display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, WHITE);
			display.display();
			delay(10);
		}
		delay(500);
		display.clearDisplay();
		display.display();
	}
}

void OLED_bitmapTest() {
	display.clearDisplay();
	display.drawBitmap(30, 16,  logo16_glcd_bmp, 16, 16, 1);
	display.display();
}

