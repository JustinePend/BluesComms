#ifndef TopLevel_OLED_h
#define TopLevel_OLED_h

//#define ARDUINO 101 
// Added above to avoid compilation warning: Adafruit_GFX.h:4:5: warning: "ARDUINO" is not defined [-Wundef]

void OLED_setup();
void OLED_CheckButtons(bool &A, bool &B, bool &C);
void OLED_graphicsTest();
void OLED_bitmapTest();
void OLED_UpdateDisplay(char* line1, char* line2, char* line3);
void OLED_OneLongString(char* dispData);


#endif  // TopLevel_OLED_h