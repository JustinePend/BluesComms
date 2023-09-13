#include "Particle.h"
#include <stdint.h>

#include "DRV8830.h"
#include "TopLevel_MCP23017.h"
#include "TopLevel_TCA9548.h"
#include "TopLevel_OLED.h"
#include "Pump_Valve_Test.h"
#include "Communications.h"

enDriverOutConf Valve_1_Dir = PORTB;
enDriverOutConf Valve_2_Dir = PORTB;
enDriverOutConf Valve_3_Dir = PORTB;

// Forward Declarations
void PVTest_setup();
void PVTest_processButtonPressEvent();
void PVTest_init_valves();
void PVTest_loop();


void PVTest_setup()
{
    TCA9548_setChannel(m_Pump_Valves);
    DRV8830_Init();

}

void PVTest_loop()
{
    PVTest_setup();

    strncpy(g_OLED_Line1, "", c_OLED_LINE_CHARS_MAX);
    strncpy(g_OLED_Line2, "Manual Control", c_OLED_LINE_CHARS_MAX);
    strncpy(g_OLED_Line3, "Press Any Button", c_OLED_LINE_CHARS_MAX);
    OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);

    while (!(g_BTN_A_PRESSED || g_BTN_B_PRESSED || g_BTN_C_PRESSED))
    {
        OLED_CheckButtons(g_BTN_A_PRESSED, g_BTN_B_PRESSED, g_BTN_C_PRESSED);
        delay(100);
    }

    PVTest_init_valves();
    delay(1000);
    DRV8830_Set_Pump(5000, FWD);

    while (1)
    {
        OLED_CheckButtons(g_BTN_A_PRESSED, g_BTN_B_PRESSED, g_BTN_C_PRESSED);
        if (g_BTN_A_PRESSED || g_BTN_B_PRESSED || g_BTN_C_PRESSED)
        {
            PVTest_processButtonPressEvent();
        }
        delay(100);
    }
}

void PVTest_processButtonPressEvent()
{
    // Note when debugging if delays are slowing down main loop
    // you may need to keep the buttons pressed down for a few seconds
    // as they are only polled once per main loop cycle
    if (g_BTN_A_PRESSED)
    {
        if (Valve_1_Dir == PORTA)
        {
            Valve_1_Dir = PORTB;
            DRV8830_Set_Valve(1, PORTB);
            snprintf(g_OLED_Line1, c_OLED_LINE_CHARS_MAX, "V1: B");
        }
        else
        {
            Valve_1_Dir = PORTA;
            DRV8830_Set_Valve(1, PORTA);
            snprintf(g_OLED_Line1, c_OLED_LINE_CHARS_MAX, "V1: A");
        }
    }

    if (g_BTN_B_PRESSED)
    {
        if (Valve_2_Dir == PORTA)
        {
            Valve_2_Dir = PORTB;
            DRV8830_Set_Valve(2, PORTB);
            snprintf(g_OLED_Line2, c_OLED_LINE_CHARS_MAX, "V2: B");
        }
        else
        {
            Valve_2_Dir = PORTA;
            DRV8830_Set_Valve(2, PORTA);
            snprintf(g_OLED_Line2, c_OLED_LINE_CHARS_MAX, "V2: A");
        }
    }

    if (g_BTN_C_PRESSED)
    {
        if (Valve_3_Dir == PORTA)
        {
            Valve_3_Dir = PORTB;
            DRV8830_Set_Valve(3, PORTB);
            snprintf(g_OLED_Line3, c_OLED_LINE_CHARS_MAX, "V3: B");
        }
        else
        {
            Valve_3_Dir = PORTA;
            DRV8830_Set_Valve(3, PORTA);
            snprintf(g_OLED_Line3, c_OLED_LINE_CHARS_MAX, "V3: A");
        }
    }

    OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
}

void PVTest_init_valves()
{
    DRV8830_Set_Valve(1, PORTB);
    delay(1000);
    DRV8830_Set_Valve(2, PORTB);
    delay(1000);
    DRV8830_Set_Valve(3, PORTB);
    snprintf(g_OLED_Line1, c_OLED_LINE_CHARS_MAX, "V1: B");
    snprintf(g_OLED_Line2, c_OLED_LINE_CHARS_MAX, "V2: B");
    snprintf(g_OLED_Line3, c_OLED_LINE_CHARS_MAX, "V3: B");
    OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
}