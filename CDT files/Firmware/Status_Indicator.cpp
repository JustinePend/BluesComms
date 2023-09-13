#include "Particle.h"
#include "Status_Indicator.h"
#include <lp55231.h>
#include "TopLevel_TCA9548.h"

static const uint8_t signal_led_brightness = 50;
static const uint8_t FET_GRN_LED = 0;
static const uint8_t FET_RED_LED = 6;
static const uint8_t Connected_GRN_LED = 4;
static const uint8_t Connected_RED_LED = 8;
static const uint8_t SDCard_GRN_LED = 2;
static const uint8_t SDCard_RED_LED = 7;

const uint8_t array[][3] = {{6, 7, 8}, {0, 2, 4}, {1, 3, 5}}; // {{R1,R2,R3},{G1,G2,G3},{B1,B2,B3},}
const uint8_t led_array[] = {6, 0, 1, 7, 2, 3, 8, 4, 5};      // {R1,G1,B1,R2,G2,B2,R3,G3,B3}

Lp55231 ledChip;

void SI_Init()
{
    TCA9548_setChannel(m_Status_Indicator);
    ledChip.Begin();
    ledChip.Enable();
    ledChip.Enable();
}

void SI_Led_AllOff()
{
    for(uint8_t i = 0; i<9; i++)
    {
        ledChip.SetChannelPWM(led_array[i], 0);
    }
}

void SI_signal_FET_Status(int *FET_Currents)
{

    //Serial.println("Fn: SI_signal_FET_Status");
    ledChip.SetChannelPWM(FET_GRN_LED, 0);
    ledChip.SetChannelPWM(FET_RED_LED, 0);
    for (int8_t i = 0; i < 8; i++)
    {
        if ((low_threshold_pA <= FET_Currents[i]) && (FET_Currents[i] < high_threshold_pA)) // Currents in pA
        {
            SI_signal_FET(GRN, 500);
            delay(250);
        }
        else
        {
            SI_signal_FET(RED, 500);
            delay(250);
        }
    } // End For
} // End Fn

void SI_signal_Cartridge_Status(int *FET_Currents)
{

    uint8_t good_count = 0;
    //Serial.println("Fn: SI_signal_Cartridge_Status");
    ledChip.SetChannelPWM(FET_GRN_LED, 0);
    ledChip.SetChannelPWM(FET_RED_LED, 0);

    for (int8_t i = 0; i < 8; i++)
    {
        if ((low_threshold_pA <= FET_Currents[i]) && (FET_Currents[i] < high_threshold_pA)) // Currents in pA
            good_count++;
    } // End For

    if(good_count >= 6)
        SI_signal_FET(GRN, 1000);
    else
        SI_signal_FET(RED, 1000);

} // End Fn

void SI_signal_FET(LED_Colour colour, uint16_t delay_ms)
{
    TCA9548_setChannel(m_Status_Indicator);
    switch (colour)
    {
    case ORG:
        ledChip.SetChannelPWM(FET_GRN_LED, signal_led_brightness);
        ledChip.SetChannelPWM(FET_RED_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(FET_GRN_LED, 0);
        ledChip.SetChannelPWM(FET_RED_LED, 0);
        break;
    case GRN:
        ledChip.SetChannelPWM(FET_GRN_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(FET_GRN_LED, 0);
        break;
    default:
        // Default is RED
        ledChip.SetChannelPWM(FET_RED_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(FET_RED_LED, 0);
    }
}

void SI_signal_SDCard_Status(LED_Colour colour, uint16_t delay_ms)
{
    TCA9548_setChannel(m_Status_Indicator);
    switch (colour)
    {
    case ORG:
        ledChip.SetChannelPWM(SDCard_GRN_LED, signal_led_brightness);
        ledChip.SetChannelPWM(SDCard_RED_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(SDCard_GRN_LED, 0);
        ledChip.SetChannelPWM(SDCard_RED_LED, 0);
        break;
    case GRN:
        ledChip.SetChannelPWM(SDCard_GRN_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(SDCard_GRN_LED, 0);
        break;
    default:
        // Default is RED
        ledChip.SetChannelPWM(SDCard_RED_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(SDCard_RED_LED, 0);
    }
}

void SI_signal_Cloud_Conn_Status(LED_Colour colour, uint16_t delay_ms)
{
    TCA9548_setChannel(m_Status_Indicator);
    switch (colour)
    {
    case ORG:
        ledChip.SetChannelPWM(Connected_GRN_LED, signal_led_brightness);
        ledChip.SetChannelPWM(Connected_RED_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(Connected_GRN_LED, 0);
        ledChip.SetChannelPWM(Connected_RED_LED, 0);
        break;
    case GRN:
        ledChip.SetChannelPWM(Connected_GRN_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(Connected_GRN_LED, 0);
        break;
    default:
        // Default is RED
        ledChip.SetChannelPWM(Connected_RED_LED, signal_led_brightness);
        delay(delay_ms);
        ledChip.SetChannelPWM(Connected_RED_LED, 0);
    }
}

void SI_Parallel_Test_RGB_Led_Colours()
{
    int8_t i = 0, j = 0;

    //Serial.println("Fn: SI_Parallel_Test_RGB_Led_Colours()");

    TCA9548_setChannel(m_Status_Indicator);
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            ledChip.SetChannelPWM(array[i][j], signal_led_brightness);
        }
        delay(1000);
        for (j = 0; j < 3; j++)
        {
            ledChip.SetChannelPWM(array[i][j], 0);
        }
        delay(500);
    }
}

void SI_cycle_RGB_Led_Colours()
{
    int8_t i = 0;

    //Serial.println("Fn: SI_cycle_RGB_Led_Colours");

    TCA9548_setChannel(m_Status_Indicator);
    for (i = 0; i < 9; i++)
    {
        ledChip.SetChannelPWM(led_array[i], signal_led_brightness);
        delay(1000);
        ledChip.SetChannelPWM(led_array[i], 0);
        delay(500);
    }
}

void SI_cycle_RGY_Led_Colours()
{
    SI_signal_FET(GRN, 1000);
    delay(500);
    SI_signal_FET(RED, 1000);
    delay(500);
    SI_signal_FET(ORG, 1000);
    delay(500);

    SI_signal_SDCard_Status(GRN, 1000);
    delay(500);
    SI_signal_SDCard_Status(RED, 1000);
    delay(500);
    SI_signal_SDCard_Status(ORG, 1000);
    delay(500);

    SI_signal_Cloud_Conn_Status(GRN, 1000);
    delay(500);
    SI_signal_Cloud_Conn_Status(RED, 1000);
    delay(500);
    SI_signal_Cloud_Conn_Status(ORG, 1000);
    delay(500);
}

void SI_signal_All_LEDs(LED_Colour colour, uint16_t delay_ms)
{
    int8_t j = 0;
    TCA9548_setChannel(m_Status_Indicator);
    switch (colour)
    {
    case ORG:
        for (j = 0; j < 3; j++)
        {
            ledChip.SetChannelPWM(array[RED][j], signal_led_brightness);
            ledChip.SetChannelPWM(array[GRN][j], signal_led_brightness);
        }
        delay(1000);
        for (j = 0; j < 3; j++)
        {
            ledChip.SetChannelPWM(array[RED][j], 0);
            ledChip.SetChannelPWM(array[GRN][j], 0);
        }
        delay(500);
        break;
    case GRN:
        for (j = 0; j < 3; j++)
        {
            ledChip.SetChannelPWM(array[GRN][j], signal_led_brightness);
        }
        delay(1000);
        for (j = 0; j < 3; j++)
        {
            ledChip.SetChannelPWM(array[GRN][j], 0);
        }
        delay(500);
        break;
    default:
        for (j = 0; j < 3; j++)
        {
            ledChip.SetChannelPWM(array[RED][j], signal_led_brightness);
        }
        delay(1000);
        for (j = 0; j < 3; j++)
        {
            ledChip.SetChannelPWM(array[RED][j], 0);
        }
        delay(500);    
        // default is red
    }
}

//Serial.println("Fn: SI_Parallel_Test_RGB_Led_Colours()");