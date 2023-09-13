#ifndef Status_Indicator_h
#define Status_Indicator_h

enum LED_Colour
{
    RED,
    GRN,
    ORG
};

const int low_threshold_pA = 10 * 1000;
const int high_threshold_pA = 150 * 1000;

void SI_Init();
void SI_Led_AllOff();
void SI_signal_All_LEDs(LED_Colour colour, uint16_t delay_ms);
void SI_signal_FET_Status(int *FET_Currents);
void SI_signal_Cartridge_Status(int *FET_Currents);

void SI_signal_FET(LED_Colour colour, uint16_t delay_ms);
void SI_signal_SDCard_Status(LED_Colour colour, uint16_t delay_ms);
void SI_signal_Cloud_Conn_Status(LED_Colour colour, uint16_t delay_ms);

void SI_cycle_RGB_Led_Colours();
void SI_cycle_RGY_Led_Colours();
void SI_Parallel_Test_RGB_Led_Colours();

#endif  // Status_Indicator_h