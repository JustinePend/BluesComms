#ifndef Communications_h
#define Communications_h

extern const size_t c_OLED_LINE_CHARS_MAX;
extern char g_OLED_Line1[];
extern char g_OLED_Line2[];
extern char g_OLED_Line3[];
extern char emptyStr[];

extern bool g_BTN_A_PRESSED;
extern bool g_BTN_B_PRESSED;
extern bool g_BTN_C_PRESSED;

void UART_Init();



#endif  // Communications_h