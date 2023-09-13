#ifndef Debug_LED_h
#define Debug_LED_h

void DBG_DebugLED_INIT();
void DBG_ToggleDebugLED();
void DBG_PulseDebugLED(uint8_t high_ms, uint8_t low_ms);
void DBG_LED_SetHigh();
void DBG_LED_SetLow();

#endif  // Debug_LED_h