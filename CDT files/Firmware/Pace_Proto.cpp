#include "Particle.h"
#define ARDUINO 101

#include <stdio.h>  // snprintf()
#include <stdint.h> // INT32_MAX
#include "Debug_LED.h"
#include "Communications.h"
#include "DRV8830.h"
#include "TopLevel_OLED.h"
#include "TopLevel_SDFAT.h"
#include "TopLevel_RTC.h"
#include "AT24_EEPROM.h"
#include "TopLevel_MCP23017.h"
#include "TopLevel_TCA9548.h"
#include "TopLevel_BME680.h"
#include "BleLogging.h"
#include "Status_Indicator.h"
#include <string.h> // strtok
#include "Pump_Valve_Test.h"
#include "EC_Sensor.h"

// Notes
// review strncpy() should last parameter be (maxLen -1) rather than
// exiting (maxLen) to allow for null at end of string

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define ADI_MEAS_EN D7

SYSTEM_THREAD(ENABLED);

SYSTEM_MODE(MANUAL); // Use for Xenon which no longer has Cloud connectivity (Mesh deprecated - used for BLE only now)
// Manual: Won't stay stuck trying to connect
//SYSTEM_MODE(AUTOMATIC); // Use for Argon / Boron

//Cellular.off();

// Constants
const unsigned long c_SEND_INTERVAL_MS = 2000;
const size_t c_READ_BUF_SIZE = 127;
const size_t c_OLED_LINE_CHARS_MAX = 23;
const size_t c_MAX_FNAME_LEN = 50;
uint8_t g_measCount = 0;
static const int c_Mins_In_One_Day = (24 * 60);

// Enums

typedef enum
{
    START_WITH_1MCP_TEST,
    PV_TEST,                  // ORG
    START_WITH_ECS_LONG_TEST, // RED
    START_WITH_ECS_CONTINUOUS // GRN
} e_FW_Mode;

e_FW_Mode g_Default_FW_Mode = START_WITH_1MCP_TEST;

typedef enum
{
    FET_POWER_ON,
    FET_BASELINE,
    FET_EXPOSURE,
    FET_DRYING,
    FET_POWER_OFF,
    FET_FINISHED
} e_FET_Meas_State;

e_FET_Meas_State g_FET_Meas_State = FET_POWER_ON;

typedef enum
{
    EC_POWER_ON,
    EC_BASELINE,
    EC_EXPOSURE,
    EC_CLEARING,
    EC_POWER_OFF,
    EC_FINISHED,
    EC_SLEEP_SYSTEM,
    EC_WAIT_INTERVAL
} e_EC_Meas_State;

e_EC_Meas_State g_EC_Meas_State = EC_POWER_ON;

// CONFIGURATION VALUES
//=======================================================================
//#define CLOUD_PUBLISH_ENABLED // Uncomment for Boron/ Argon

// check SYSTEM_MODE setting Default: Automatic (Boron/Argon),  Manual (Xenon)
#define g_KitNum 11
// See set_DefaultStartupMode for Kit Number Specific Special Startup Mode definitions
#define g_KitType "_X" // First letter of board type e.g. "B" Boron
static const uint16_t g_FW_Num = 3001; 

#define PACE_STANDARD_TEST
//#define CDT_STANDARD_TEST
//#define CDT_FAST_TEST

// c_EC_WAIT_MINS: relavant for kits 1-5 (These do not have the Power Shutdown Controller Board Fitted)

#ifdef PACE_STANDARD_TEST
// Comment standard values in []
static const int c_FET_BASELINE_COUNT = (45 * 2 - 1); // [45] min
static const int c_FET_EXPOSURE_COUNT = (15 * 2 - 1); // [15] min
static const int c_FET_MEAS_CYCLES = (24 - 1);        // [24] Cycles
static const int c_FET_DRYING_COUNT = (30 * 2 - 1);   // [30] min

static const int c_EC_BASELINE_COUNT = (12 * 6 - 1);               // [12] min
static const int c_EC_EXPOSURE_COUNT = (3 * 6 - 1);                // [3] min
static const int c_EC_MEAS_CYCLES = (4- 1);                       // [3] Cycles
static const int c_EC_CLEARING_COUNT = (5 * 6 - 1);                // [5] min
static const int c_EC_SLEEP_HRS = 47;                              //[47] hrs = 2 Days (Subtract off ~ 1h test duration)
static const int c_EC_WAIT_MINS = (2 * c_Mins_In_One_Day); // [2] days
#endif

#ifdef CDT_STANDARD_TEST
static const int c_FET_BASELINE_COUNT = (45 * 2 - 1); // [45] min
static const int c_FET_EXPOSURE_COUNT = (15 * 2 - 1); // [15] min
static const int c_FET_MEAS_CYCLES = (24 - 1);        // [24] Cycles
static const int c_FET_DRYING_COUNT = (30 * 2 - 1);   // [30] min

static const int c_EC_BASELINE_COUNT = (12 * 6 - 1);               // [12] min
static const int c_EC_EXPOSURE_COUNT = (3 * 6 - 1);                // [3] min
static const int c_EC_MEAS_CYCLES = (48 - 1);                       // [3] Cycles
static const int c_EC_CLEARING_COUNT = (5 * 6 - 1);                // [5] min
static const int c_EC_SLEEP_HRS = 1;                               //[167] 7 days = 167h
static const int c_EC_WAIT_MINS = 60; // [7] days
#endif

#ifdef CDT_FAST_TEST
static const int c_FET_BASELINE_COUNT = (10 * 2 - 1); // [45] min
static const int c_FET_EXPOSURE_COUNT = (5 * 2 - 1); // [15] min
static const int c_FET_MEAS_CYCLES = (24 - 1);        // [24] Cycles
static const int c_FET_DRYING_COUNT = (30 * 2 - 1);   // [30] min

static const int c_EC_BASELINE_COUNT = (4 * 6 - 1); // [12] min
static const int c_EC_EXPOSURE_COUNT = (1 * 6 - 1); // [3] min
static const int c_EC_MEAS_CYCLES = (4 - 1);        // [3] Cycles
static const int c_EC_CLEARING_COUNT = (1 * 6 - 1); // [5] min
static const int c_EC_SLEEP_HRS = 1;                //[167] 7 days = 167h
static const int c_EC_WAIT_MINS = 5;     // [7] days
// CDT FAST TEST
// EC_SLEEP_HRS replaced by 1 min sleep
// Cycle LEDs and audible valve test skipped
#endif

//=======================================================================
#define kitSubStr "Kit" STR(g_KitNum)
#define g_kitNumStr STR(g_KitNum) \
g_KitType

#define Ch1SubStr "Ch1"
#define Ch2SubStr "Ch2"
#define Ch3SubStr "Ch3"
#define Ch1Str kitSubStr Ch1SubStr
#define Ch2Str kitSubStr Ch2SubStr
#define Ch3Str kitSubStr Ch3SubStr

// Forward declarations
void setup();
void loop();
void clearSerialBuffer();
void processButtonPressEvent();
void processBuffer();

void getSensorData();
void timerEventHandler();
void zeroSensorData();
void selectExternalMeshAntenna();
void readAnalogInputs();
void read_BME680();
e_FET_Meas_State update_GSFET_Meas_State(e_FET_Meas_State inState);

e_EC_Meas_State update_EC_Meas_State(e_EC_Meas_State inState);
void Change_Valves_State(enDriverOutConf valve1_dir, enDriverOutConf valve2_dir, enDriverOutConf valve3_dir);

void extractResValues(char *resStr);
void read_Mode_SW();
void perform_EC_Meas();
void switchOverTo_ECS_measurments();

void genPublishStrings();
void publishDataToCloud();
void triggerSystemSleepMins(uint8_t minsToSleep);
void triggerSystemSleepHrs(uint8_t hrsToSleep);
void read_FirstPowerUp_Sig_Line();
void logStartupMode(e_FW_Mode fw_Mode);
void logParameterSetValues();
void logCommonDetails();
void set_numECmeasCycles(e_FW_Mode fw_Mode);
void set_DefaultStartupMode(uint8_t kitNum);
void set_ECS_VOC_ETH_Alternation_Mode(uint8_t kitNum);

// Global variables
int g_counter = 100000;
uint8_t g_debug_count = 0;
unsigned long g_lastSend = 0;
bool g_SD_Logging_Enable = true;
bool g_ADuCM360_Meas_Enable = false;
int g_AnalogValue = 0;
e_FW_Mode g_FW_Mode = g_Default_FW_Mode;
int g_EC_Meas_Count = 0;

char g_readBuf[c_READ_BUF_SIZE];
char g_readBufCopy[c_READ_BUF_SIZE];
char g_dataLine[c_READ_BUF_SIZE];
const char c_tapSeperator = '@';
size_t g_readBufOffset = 0;

char g_OLED_Line1[c_OLED_LINE_CHARS_MAX] = "";
char g_OLED_Line2[c_OLED_LINE_CHARS_MAX] = "";
char g_OLED_Line3[c_OLED_LINE_CHARS_MAX] = "";
char emptyStr[] = "";

const int msgStrLen = 256;
char msgStr1[msgStrLen], msgStr2[msgStrLen], msgStr3[msgStrLen];

const char g_prefixStr_ECS_Data[] = "ECS";
const char g_prefixStr_FET_Data[] = "FET";
const char g_prefixStr_LOG_Data[] = "LOG";

bool g_BTN_A_PRESSED = false;
bool g_BTN_B_PRESSED = false;
bool g_BTN_C_PRESSED = false;

bool bFirst_Power_Up = true;

bool g_bPSC_Board_Fitted = true;
bool g_bECS_Alt_VOC_ETH = true;

char g_FName[c_MAX_FNAME_LEN] = "";

char g_Cartridge_Num = 0;

int32_t g_EC_Meas_Cycles = 1;

BleLogging<4096> bleLogHandler(LOG_LEVEL_INFO);

struct dataStruct
{
    int32_t I1;
    int32_t I2;
    int32_t I3;
    int32_t I4;
    int32_t I5;
    int32_t I6;
    int32_t I7;
    int32_t I8;
    int16_t VG_V;
    float BME680_Temp;
    float BME680_Hum;
    float BME680_kOhms;
    float BME680_Press_hPa;
    float BAT1_Volts;
    float BAT2_Volts;
    uint32_t measNum;
    uint8_t ValvesState;
    float EC_Sens;
} sensorData;

void setup()
{
    if (g_KitNum > 5)
        g_bPSC_Board_Fitted = true;
    else
        g_bPSC_Board_Fitted = false;

    MCP23017_setup(g_bPSC_Board_Fitted);
    MCP_set_low(mcp_nSHDN_7V);

    digitalWrite(ADI_MEAS_EN, LOW);
    pinMode(ADI_MEAS_EN, OUTPUT);
    pinMode(A1, INPUT);
    pinMode(A2, INPUT);

    UART_Init();
    clearSerialBuffer();
    OLED_setup();
    OLED_graphicsTest();
    RTC_Init();

    TCA9548_Init();

    SI_Init();

    RTC_GenDataFileName(g_prefixStr_LOG_Data, g_kitNumStr, c_MAX_FNAME_LEN, g_FName);

    set_DefaultStartupMode(g_KitNum);
    set_ECS_VOC_ETH_Alternation_Mode(g_KitNum);
    read_Mode_SW();

    delay(1000);

    if (g_bPSC_Board_Fitted)
        read_FirstPowerUp_Sig_Line();

    if (g_FW_Mode == PV_TEST)
        PVTest_loop(); // Never Returns; Control passed to this alternate mode

    SPI.begin();
    ECS_Init();
    delay(1000);

    g_Cartridge_Num = AT24_EEPROM_Get_Cartridge_Number();

    sprintf(g_OLED_Line1, "Cartridge: %d", g_Cartridge_Num);
    sprintf(g_OLED_Line2, "Kit %s FW: %d", g_kitNumStr, g_FW_Num);
    strncpy(g_OLED_Line3, "CDT GAS SENSOR KIT", c_OLED_LINE_CHARS_MAX);
    OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);

    delay(1000);

    set_numECmeasCycles(g_FW_Mode);

    logCommonDetails();
    logStartupMode(g_FW_Mode);
    logParameterSetValues();

    RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
    OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, emptyStr);

    TCA9548_setChannel(m_Pump_Valves);
    DRV8830_Init();    
#ifndef CDT_FAST_TEST
    strncpy(g_OLED_Line3, "Audible P/V Test", c_OLED_LINE_CHARS_MAX);
    OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
    DRV8830_Audible_Valve_Test(0); // 0: excercise the valves but don't run the pump

    SI_cycle_RGY_Led_Colours();
#endif
    delay(1000);

    delay(1000); // Allow some time for large caps on Pump/Valve to charge

    BME680_Init();

    //selectExternalMeshAntenna();
    bleLogHandler.setup();
    Serial.printlnf("FW: %d",g_FW_Num);

    if ((g_FW_Mode == START_WITH_ECS_LONG_TEST) ||
        (g_FW_Mode == START_WITH_ECS_CONTINUOUS) ||
        (bFirst_Power_Up == false))
        switchOverTo_ECS_measurments(); // Never returns (includes a While(1) loop)

    MCP_set_high(mcp_nSHDN_7V); // Power-up ADICUP Board
    g_FET_Meas_State = update_GSFET_Meas_State(FET_POWER_ON);
}

void loop()
{

    // Read data from serial
    bleLogHandler.loop();
    //The receive buffer size for hardware serial channels (Serial1, Serial2) is 64 bytes and cannot be changed.
    while (Serial1.available())
    {
        if (g_readBufOffset < c_READ_BUF_SIZE)
        {
            char c = Serial1.read();
            if (c != '\n')
            {
                // Add character to buffer
                g_readBuf[g_readBufOffset++] = c;
            }
            else
            {
                // End of line character found, process line
                // Serial.printlnf("%d", g_readBufOffset);
                g_readBuf[g_readBufOffset] = 0;
                processBuffer();
                Serial.print("[BUF]: ");
                Serial.println(g_readBuf);

                g_readBufCopy[0] = 0;
                strncpy(g_readBufCopy, g_readBuf, c_READ_BUF_SIZE);

                extractResValues(g_readBufCopy);
                // Extract from copy as strtok modifies the buffer
                // The original buffer needs to be maintained
                // in case the user presses button c to see values on OLED display

                g_FET_Meas_State = update_GSFET_Meas_State(g_FET_Meas_State);
                g_readBufOffset = 0;
            }
        }
        else
        {
            Serial.println("g_readBuf overflow, emptying buffer");
            g_readBufOffset = 0;
        } // End If-Else
    }     // End While

    OLED_CheckButtons(g_BTN_A_PRESSED, g_BTN_B_PRESSED, g_BTN_C_PRESSED);
    processButtonPressEvent();

    if (g_FET_Meas_State == FET_POWER_OFF)
    {
        // If state is power-off no more readings will come through over serial
        // The following call will advance the state to Finished
        g_FET_Meas_State = update_GSFET_Meas_State(g_FET_Meas_State);
    }

    if (g_FET_Meas_State == FET_FINISHED)
    {
        switchOverTo_ECS_measurments(); // Never returns While(1) loop inside
    }                                   // End If

} // End Loop

void processBuffer()
{
    uint32_t fileSize = 0;
    static uint32_t measNum = 0;

    Serial.printlnf("Received from ADuCM360: %s", g_readBuf);
    RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);

    read_BME680();
    readAnalogInputs();

    if (g_SD_Logging_Enable)
    {
        sensorData.measNum = measNum;
        sprintf(g_dataLine, "%s\t%ld", g_OLED_Line1, sensorData.measNum);
        fileSize = SD_WriteData(g_dataLine, g_FName, FALSE);

        sprintf(g_dataLine, "\t%s", g_readBuf);
        fileSize = SD_WriteData(g_dataLine, g_FName, FALSE);

        snprintf(g_OLED_Line2, c_OLED_LINE_CHARS_MAX, "SD FSize(B): %lu", fileSize);

        sensorData.ValvesState = DRV8830_read_Valves_Config();

        sprintf(g_dataLine, "\t%d\t%f\t%f\t%f\t%f",
                sensorData.ValvesState,
                sensorData.BME680_Temp,
                sensorData.BME680_Hum,
                sensorData.BME680_Press_hPa,
                sensorData.BME680_kOhms);
        Serial.print("#1[");
        Serial.print(g_dataLine);
        Serial.printlnf("]");
        fileSize = SD_WriteData(g_dataLine, g_FName, FALSE);

        if (fileSize > 0)
            SI_signal_SDCard_Status(GRN, 500);
        else
            SI_signal_SDCard_Status(RED, 500);

#ifdef CLOUD_PUBLISH_ENABLED
        if (Particle.connected())
        {
            SI_signal_Cloud_Conn_Status(GRN, 500);
            publishDataToCloud();
        }
        else
            SI_signal_Cloud_Conn_Status(RED, 500);
#else
        SI_signal_Cloud_Conn_Status(RED, 500);
#endif

        sprintf(g_dataLine, "\t%f\t%f",
                sensorData.BAT1_Volts,
                sensorData.BAT2_Volts);
        Serial.print("#2[");
        Serial.print(g_dataLine);
        Serial.printlnf("]");
        fileSize = SD_WriteData(g_dataLine, g_FName, TRUE);

        measNum++;
    }

    char subbuff[7];
    memcpy(subbuff, &g_readBuf[7], 6); // 6 digits + '\t' -> Ch2 start index = 7
    subbuff[6] = '\0';

    strncpy(g_OLED_Line3, "Ch2 (pA): ", 12);
    strncat(g_OLED_Line3, subbuff, 6);

    OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
    Log.printf("\n\n%s\n", g_OLED_Line1);
    Log.printf("Meas Num %ld\n", measNum);
    Log.printf("%s\n", g_readBuf);
    Log.printf("SD FSize %ld\n", fileSize);
    Log.printf("BT %3.2f\n", sensorData.BME680_Temp);
    Log.printf("BRH %3.2f\n", sensorData.BME680_Hum);
    Log.printf("BP %3.2f\n", sensorData.BME680_Press_hPa);
    Log.printf("BR %3.2f\n", sensorData.BME680_kOhms);
    Log.printf("V1 %3.2f\n", sensorData.BAT1_Volts);
    Log.printf("V2 %3.2f\n", sensorData.BAT2_Volts);

    zeroSensorData();

    //DBG_PulseDebugLED(3,1);
}

void processButtonPressEvent()
{
    // Note when debugging if delays are slowing down main loop
    // you may need to keep the buttons pressed down for a few seconds
    // as they are only polled once per main loop cycle
    if (g_BTN_A_PRESSED)
    {
        RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
        getSensorData();
        snprintf(g_OLED_Line2, c_OLED_LINE_CHARS_MAX, "BV/P %3.2f %3.2f",
                 sensorData.BAT1_Volts,
                 sensorData.BME680_Press_hPa);
        Serial.printlnf("%s", g_OLED_Line2);
        snprintf(g_OLED_Line3, c_OLED_LINE_CHARS_MAX, "B %3.2f %3.2f %3.2f",
                 sensorData.BME680_Temp,
                 sensorData.BME680_Hum,
                 sensorData.BME680_kOhms);
        Serial.printlnf("%s", g_OLED_Line3);
        OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
    }

    if (g_BTN_B_PRESSED)
    {
        if (g_SD_Logging_Enable)
        {
            g_SD_Logging_Enable = false;
            strncpy(g_OLED_Line2, "SD LOGGING STOPPED", c_OLED_LINE_CHARS_MAX);
            OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, emptyStr);
        }
        else
        {
            g_SD_Logging_Enable = true;
            strncpy(g_OLED_Line2, "SD LOGGING ENABLED", c_OLED_LINE_CHARS_MAX);
            OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, emptyStr);
        }
    }

    if (g_BTN_C_PRESSED)
    {
        OLED_OneLongString(g_readBuf);
    }
}

void clearSerialBuffer()
{
    while (Serial1.available())
        Serial1.read();
}

void readAnalogInputs()
{
    // 6.xyz: Calibration value determined by measuring with DMM
    g_AnalogValue = analogRead(A1);
    sensorData.BAT1_Volts = (((((float)g_AnalogValue) * 3.3) / 4095) * 6.112);
    g_AnalogValue = analogRead(A2);
    sensorData.BAT2_Volts = (((((float)g_AnalogValue) * 3.3) / 4095) * 6.112);

    Serial.print("SMPS (V): ");
    Serial.print(sensorData.BAT1_Volts);
    Serial.print(" ");
    Serial.print(sensorData.BAT2_Volts);
}

void getSensorData()
{
    read_BME680();
    readAnalogInputs();
} // End Function

void zeroSensorData()
{
    //Serial.println("zeroSensorData()");
    sensorData.I1 = 0;
    sensorData.I2 = 0;
    sensorData.I3 = 0;
    sensorData.I4 = 0;
    sensorData.I5 = 0;
    sensorData.I6 = 0;
    sensorData.I7 = 0;
    sensorData.I8 = 0;
    sensorData.VG_V = 0;
    sensorData.BME680_Temp = 0;
    sensorData.BME680_Hum = 0;
    sensorData.BME680_kOhms = 0;
    sensorData.BME680_Press_hPa = 0;
    sensorData.BAT1_Volts = 0;
    sensorData.BAT2_Volts = 0;
    sensorData.measNum = 0;
    sensorData.ValvesState = 0;
    sensorData.EC_Sens = 0;
}

void selectExternalMeshAntenna()
{

#if (PLATFORM_ID == PLATFORM_ARGON)
    digitalWrite(ANTSW1, 1);
    digitalWrite(ANTSW2, 0);
#elif (PLATFORM_ID == PLATFORM_BORON)
    digitalWrite(ANTSW1, 0);
#else
    digitalWrite(ANTSW1, 0);
    digitalWrite(ANTSW2, 1);
#endif
}

e_FET_Meas_State update_GSFET_Meas_State(e_FET_Meas_State inState)
{
    static uint8_t inStateCount = 0;
    static uint8_t cyclesCompleted = 0;
    e_FET_Meas_State outState;

    outState = inState; // Default

    switch (inState)
    {
    case FET_POWER_ON:
        RTC_GenDataFileName(g_prefixStr_FET_Data, g_kitNumStr, c_MAX_FNAME_LEN, g_FName);
        delay(10000); // After power up allow time for caps to charge
        Change_Valves_State(PORTB, PORTA, PORTB);
        RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
        sprintf(g_dataLine, "# %s FET_POWER_ON; PUMP ON; BAB", g_OLED_Line1);
        SD_WriteData(g_dataLine, g_FName, TRUE);
        strncpy(g_OLED_Line3, "PUMP ON BAB", c_OLED_LINE_CHARS_MAX);
        OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        digitalWrite(ADI_MEAS_EN, HIGH);
        outState = FET_BASELINE;
        break;
    case FET_BASELINE:
        if (inStateCount == 0)
        {
            digitalWrite(ADI_MEAS_EN, LOW);
            Change_Valves_State(PORTB, PORTB, PORTB);
            digitalWrite(ADI_MEAS_EN, HIGH);
            RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
            sprintf(g_dataLine, "# %s FET_BASELINE BBB", g_OLED_Line1);
            SD_WriteData(g_dataLine, g_FName, TRUE);
            strncpy(g_OLED_Line3, "FET_BASELINE BBB", c_OLED_LINE_CHARS_MAX);
            OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        }
        inStateCount++;
        if (inStateCount > c_FET_BASELINE_COUNT)
        {
            inStateCount = 0;
            outState = FET_EXPOSURE;
        }
        break;
    case FET_EXPOSURE:
        if (inStateCount == 0)
        {
            digitalWrite(ADI_MEAS_EN, LOW);
            Change_Valves_State(PORTA, PORTB, PORTB);
            digitalWrite(ADI_MEAS_EN, HIGH);
            RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
            sprintf(g_dataLine, "# %s FET_EXPOSURE ABB CYCLE: %d", g_OLED_Line1, cyclesCompleted);
            SD_WriteData(g_dataLine, g_FName, TRUE);
            strncpy(g_OLED_Line3, "FET_EXPOSURE ABB", c_OLED_LINE_CHARS_MAX);
            OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        }
        inStateCount++;
        if (inStateCount > c_FET_EXPOSURE_COUNT) //0-29: 15 min elapsed
        {
            inStateCount = 0;
            cyclesCompleted++;
            if (cyclesCompleted > c_FET_MEAS_CYCLES)
            {
                outState = FET_DRYING;
            }
            else
            {
                outState = FET_BASELINE;
            }
        }
        break;
    case FET_DRYING:
        if (inStateCount == 0)
        {
            digitalWrite(ADI_MEAS_EN, LOW);
            Change_Valves_State(PORTB, PORTA, PORTA);
            digitalWrite(ADI_MEAS_EN, HIGH);
            RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
            sprintf(g_dataLine, "# %s FET_DRYING BAA", g_OLED_Line1);
            SD_WriteData(g_dataLine, g_FName, TRUE);
            strncpy(g_OLED_Line3, "FET_DRYING BAA", c_OLED_LINE_CHARS_MAX);
            OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        }
        inStateCount++;
        if (inStateCount > c_FET_DRYING_COUNT)
        {
            inStateCount = 0;
            outState = FET_POWER_OFF;
        }
        break;
    case FET_POWER_OFF:
        Change_Valves_State(PORTB, PORTA, PORTB);
        digitalWrite(ADI_MEAS_EN, LOW);
        MCP_set_low(mcp_nSHDN_7V); // Power Off ADICUP Board
        TCA9548_setChannel(m_Pump_Valves);
        DRV8830_Set_Pump(0, FWD);
        delay(1000);
        RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
        sprintf(g_dataLine, "# %s FET_POWER_OFF BAB", g_OLED_Line1);
        SD_WriteData(g_dataLine, g_FName, TRUE);
        strncpy(g_OLED_Line3, "POWER-OFF BAB", c_OLED_LINE_CHARS_MAX);
        OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        inStateCount = 0;
        outState = FET_FINISHED;
        break;
    case FET_FINISHED:
        RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
        sprintf(g_dataLine, "# %s FET_FINISHED", g_OLED_Line1);
        SD_WriteData(g_dataLine, g_FName, TRUE);
        strncpy(g_OLED_Line3, "FET_FINISHED", c_OLED_LINE_CHARS_MAX);
        OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        outState = FET_FINISHED;
        break;
    default:
        outState = FET_FINISHED;
    }
    sprintf(g_dataLine, "GSFET_MS:\t%d\t%d\t%d\t%d",
            inState,
            outState,
            inStateCount,
            cyclesCompleted);
    Serial.printlnf("%s", g_dataLine);
    return (outState);
}

void read_BME680()
{
    BME680_UpdateData();
    sensorData.BME680_Temp = bme.temperature;
    sensorData.BME680_Press_hPa = bme.pressure / 100.0;
    sensorData.BME680_Hum = bme.humidity;
    sensorData.BME680_kOhms = bme.gas_resistance / 1000.0;

    Serial.print("BME680: ");
    Serial.print(sensorData.BME680_Temp);
    Serial.print(" ");
    Serial.print(sensorData.BME680_Hum);
    Serial.print(" ");
    Serial.print(sensorData.BME680_Press_hPa);
    Serial.print(" ");
    Serial.println(sensorData.BME680_kOhms);
}

void extractResValues(char *resStr)
{
    Serial.println("extractResValues()");
    const char s[] = "\t";
    char *token;
    const int ARRAY_LEN = 9; // 8x FET Currents, BiasState H/L
    int valuesArray[ARRAY_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    int i = 0;
    int x = 0;

    /* get the first token */
    token = strtok(resStr, s);

    /* walk through other tokens */
    while (token != NULL)
    {
        x = atoi(token);
        if (i < ARRAY_LEN)
            valuesArray[i] = x;

        token = strtok(NULL, s);
        i++;
    }

    /*for (i = 0; i < ARRAY_LEN; i++)
    {
        Serial.println(valuesArray[i]);
    }*/

    if (x > 0)
    {
        // last token read = MeasBias H/L
        // Only signal FET Currents for H (-5V data)
        SI_signal_FET_Status(valuesArray);
        sensorData.VG_V = -5000;
        //SI_signal_Cartridge_Status(valuesArray);
    }
    else
        sensorData.VG_V = -4500;

    sensorData.I1 = valuesArray[0];
    sensorData.I2 = valuesArray[1];
    sensorData.I3 = valuesArray[2];
    sensorData.I4 = valuesArray[3];
    sensorData.I5 = valuesArray[4];
    sensorData.I6 = valuesArray[5];
    sensorData.I7 = valuesArray[6];
    sensorData.I8 = valuesArray[7];
}

void Change_Valves_State(enDriverOutConf valve1_dir, enDriverOutConf valve2_dir, enDriverOutConf valve3_dir)
{
    TCA9548_setChannel(m_Pump_Valves);
    DRV8830_Set_Pump(0, FWD);
    delay(1000);
    DRV8830_Set_All_Valves(valve1_dir, valve2_dir, valve3_dir);
    delay(1000);
    DRV8830_Set_All_Valves(valve1_dir, valve2_dir, valve3_dir);
    delay(1000);
    DRV8830_Set_Pump(5000, FWD);
}

void read_Mode_SW()
{
    // There is a pull up on the mode button line
    // If the used presses the mode button the line will read as LOW

    if (MCP_read_input(mcp_EXT_STATUS_SW) == HIGH)
    {
        g_FW_Mode = g_Default_FW_Mode;
        Serial.println("Mode Switch Read as: g_Default_FW_Mode");
        return;
    }

    delay(1000);
    g_FW_Mode = PV_TEST;
    SI_signal_All_LEDs(ORG, 2000);
    delay(2000);
    if (MCP_read_input(mcp_EXT_STATUS_SW) == HIGH)
    {
        Serial.println("Mode Switch Read as: PV_TEST");
        return;
    }

    g_FW_Mode = START_WITH_ECS_LONG_TEST;
    SI_signal_All_LEDs(RED, 2000);
    delay(2000);
    if (MCP_read_input(mcp_EXT_STATUS_SW) == HIGH)
    {
        Serial.println("Mode Switch Read as: START_WITH_ECS_LONG_TEST");
        return;
    }

    g_FW_Mode = START_WITH_ECS_CONTINUOUS;
    SI_signal_All_LEDs(GRN, 2000);
    delay(2000);
    if (MCP_read_input(mcp_EXT_STATUS_SW) == HIGH)
    {
        Serial.println("Mode Switch Read as: START_WITH_ECS_CONTINUOUS");
        return;
    }

    g_FW_Mode = g_Default_FW_Mode;
    Serial.println("Mode Switch Read as: g_Default_FW_Mode");
    return;
}

e_EC_Meas_State update_EC_Meas_State(e_EC_Meas_State inState)
{
    static uint8_t inStateCount = 0;
    static uint8_t cyclesCompleted = 0;
    e_EC_Meas_State outState;

    int minsWaited = 0;
    uint32_t next = millis();

    outState = inState; // Default

    switch (inState)
    {
    case EC_POWER_ON:
        delay(10000); // After power up allow time for caps to charge
        cyclesCompleted = 0;
        g_EC_Meas_Count = 0;
        inStateCount = 0;

        SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE3));
        ECS_Init();
        SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE0));

        RTC_GenDataFileName(g_prefixStr_ECS_Data, g_kitNumStr, c_MAX_FNAME_LEN, g_FName);

        sprintf(g_dataLine, "# FW: %d", g_FW_Num);
        SD_WriteData(g_dataLine, g_FName, TRUE);

        sprintf(g_dataLine, "# Kit Number: %s", g_kitNumStr);
        SD_WriteData(g_dataLine, g_FName, TRUE);

        sprintf(g_dataLine, "# Cartridge Number: %d", g_Cartridge_Num);
        SD_WriteData(g_dataLine, g_FName, TRUE);

        RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
        sprintf(g_dataLine, "# %s ECS MEAS START", g_OLED_Line1);
        SD_WriteData(g_dataLine, g_FName, TRUE);

        sprintf(g_dataLine, "# ECS Cycles Setting: %ld", g_EC_Meas_Cycles);
        SD_WriteData(g_dataLine, g_FName, TRUE);        

        Change_Valves_State(PORTB, PORTA, PORTA);
        RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
        sprintf(g_dataLine, "# %s BAA", g_OLED_Line1);
        SD_WriteData(g_dataLine, g_FName, TRUE);

        strncpy(g_OLED_Line3, "EC PUMP ON BAA", c_OLED_LINE_CHARS_MAX);
        OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        outState = EC_BASELINE;
        Serial.println("EC_POWER_ON");
        break;
    case EC_BASELINE:
        if (inStateCount == 0)
        {
            Change_Valves_State(PORTB, PORTA, PORTA);
            RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
            sprintf(g_dataLine, "# %s EC_BASELINE BAA", g_OLED_Line1);
            SD_WriteData(g_dataLine, g_FName, TRUE);
            strncpy(g_OLED_Line3, "EC_BASELINE BAA", c_OLED_LINE_CHARS_MAX);
            OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        }

        inStateCount++;
        if (inStateCount > c_EC_BASELINE_COUNT)
        {
            inStateCount = 0;
            outState = EC_EXPOSURE;
        }
        Serial.println("EC_BASELINE");
        break;
    case EC_EXPOSURE:
        if (inStateCount == 0)
        {
            if((cyclesCompleted % 2) && g_bECS_Alt_VOC_ETH)
                Change_Valves_State(PORTB, PORTB, PORTA);
            else
                Change_Valves_State(PORTA, PORTA, PORTA);

            RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
            sprintf(g_dataLine, "# %s EC_EXPOSURE AAA CYCLE: %d", g_OLED_Line1, cyclesCompleted);
            SD_WriteData(g_dataLine, g_FName, TRUE);
            strncpy(g_OLED_Line3, "EC_EXPOSURE AAA", c_OLED_LINE_CHARS_MAX);
            OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        }
        inStateCount++;
        if (inStateCount > c_EC_EXPOSURE_COUNT)
        {
            inStateCount = 0;
            cyclesCompleted++;

            if (cyclesCompleted > g_EC_Meas_Cycles)
            {
                outState = EC_CLEARING;
            }
            else
            {
                outState = EC_BASELINE;
            }
        }
        Serial.println("EC_EXPOSURE");
        break;
    case EC_CLEARING:
        if (inStateCount == 0)
        {
            Change_Valves_State(PORTB, PORTA, PORTA);
            RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
            sprintf(g_dataLine, "# %s EC_CLEARING BAA", g_OLED_Line1);
            SD_WriteData(g_dataLine, g_FName, TRUE);
            strncpy(g_OLED_Line3, "EC_CLEARING BAA", c_OLED_LINE_CHARS_MAX);
            OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        }
        inStateCount++;
        if (inStateCount > c_EC_CLEARING_COUNT)
        {
            inStateCount = 0;
            outState = EC_POWER_OFF;
        }
        Serial.println("EC_CLEARING");
        break;
    case EC_POWER_OFF:
        Change_Valves_State(PORTB, PORTA, PORTB);
        TCA9548_setChannel(m_Pump_Valves);
        DRV8830_Set_Pump(0, FWD);
        delay(1000);
        RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
        sprintf(g_dataLine, "# %s EC_POWER_OFF BAB", g_OLED_Line1);
        SD_WriteData(g_dataLine, g_FName, TRUE);
        strncpy(g_OLED_Line3, "POWER-OFF BAB", c_OLED_LINE_CHARS_MAX);
        OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        inStateCount = 0;
        Serial.println("EC_POWER_OFF");
        outState = EC_FINISHED;
        break;
    case EC_FINISHED:
        RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
        sprintf(g_dataLine, "# %s EC_FINISHED", g_OLED_Line1);
        SD_WriteData(g_dataLine, g_FName, TRUE);
        strncpy(g_OLED_Line3, "EC_FINISHED", c_OLED_LINE_CHARS_MAX);
        OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
        Serial.println("EC_FINISHED");
        g_EC_Meas_Cycles = c_EC_MEAS_CYCLES; // Sets num cycles to standard value for future repeats
        if(g_bPSC_Board_Fitted)
            outState = EC_SLEEP_SYSTEM;
        else
            outState = EC_WAIT_INTERVAL;
        break;
    case EC_SLEEP_SYSTEM:
        Serial.println("EC_SLEEP_SYSTEM");
#ifdef CDT_FAST_TEST
        triggerSystemSleepMins(1);
#else
        triggerSystemSleepHrs(c_EC_SLEEP_HRS);
#endif
        while (1)
        {
            delay(1000);
            Serial.print(".");
        }
        outState = EC_WAIT_INTERVAL;  // This line will not be reached                                                              
        break;
    case EC_WAIT_INTERVAL:
        sprintf(g_OLED_Line2, "EC_WAIT_INTERVAL");
        next = millis() + 60000;
        Serial.println("EC_WAIT_INTERVAL");
        minsWaited = 0;
        while (minsWaited < c_EC_WAIT_MINS)
        {
            if (millis() >= next)
            {
                RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
                sprintf(g_OLED_Line3, "%d / %d", minsWaited, c_EC_WAIT_MINS);
                OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);
                Serial.println(g_OLED_Line3);
                next += 60000;
                minsWaited++;
            }
            delay(1000);
        }
        outState = EC_POWER_ON;
        break;
    default:
        outState = EC_FINISHED;
    }
    sprintf(g_dataLine, "EC_MS:\t%d\t%d\t%d\t%d",
            inState,
            outState,
            inStateCount,
            cyclesCompleted);
    Serial.printlnf("%s", g_dataLine);
    return (outState);
}

void perform_EC_Meas()
{
    float fEth_ppm = 0;
    uint32_t fileSize = 0;

    zeroSensorData();

    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE3));
    fEth_ppm = ECS_Get_Reading();
    SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE0));

    RTC_GetTime(g_OLED_Line1, c_OLED_LINE_CHARS_MAX);
    snprintf(g_OLED_Line2, c_OLED_LINE_CHARS_MAX, "PPM: %f", fEth_ppm);
    snprintf(g_OLED_Line3, c_OLED_LINE_CHARS_MAX, "%d", g_EC_Meas_Count);
    OLED_UpdateDisplay(g_OLED_Line1, g_OLED_Line2, g_OLED_Line3);

    sensorData.measNum = g_EC_Meas_Count;
    sensorData.EC_Sens = fEth_ppm;

    sprintf(g_dataLine, "%s\t%ld\t%f", g_OLED_Line1, sensorData.measNum, sensorData.EC_Sens);
    fileSize = SD_WriteData(g_dataLine, g_FName, FALSE);

    Serial.print(g_EC_Meas_Count);
    Serial.print('\t');
    Serial.println(fEth_ppm, 6);

    getSensorData();

    sensorData.ValvesState = DRV8830_read_Valves_Config();

    sprintf(g_dataLine, "\t%d\t%f\t%f\t%f\t%f",
            sensorData.ValvesState,
            sensorData.BME680_Temp,
            sensorData.BME680_Hum,
            sensorData.BME680_Press_hPa,
            sensorData.BME680_kOhms);
    Serial.print("#1[");
    Serial.print(g_dataLine);
    Serial.printlnf("]");
    fileSize = SD_WriteData(g_dataLine, g_FName, FALSE);

    sprintf(g_dataLine, "\t%f\t%f",
            sensorData.BAT1_Volts,
            sensorData.BAT2_Volts);
    Serial.print("#2[");
    Serial.print(g_dataLine);
    Serial.printlnf("]");
    fileSize = SD_WriteData(g_dataLine, g_FName, TRUE);

    if (fileSize > 0)
        SI_signal_SDCard_Status(GRN, 500);
    else
        SI_signal_SDCard_Status(RED, 500);

    g_EC_Meas_Count++;

#ifdef CLOUD_PUBLISH_ENABLED
    if (Particle.connected())
    {
        SI_signal_Cloud_Conn_Status(GRN, 500);
        publishDataToCloud();
    }
    else
        SI_signal_Cloud_Conn_Status(RED, 500);
#else
    SI_signal_Cloud_Conn_Status(RED, 500);
#endif

    Log.printf("\n\nECS MODE\n");
    Log.printf("Meas Num %d\n", g_EC_Meas_Count);
    Log.printf("ETH %f\n", fEth_ppm);
    Log.printf("SD FSize %ld\n", fileSize);
    Log.printf("BT %3.2f\n", sensorData.BME680_Temp);
    Log.printf("BRH %3.2f\n", sensorData.BME680_Hum);
    Log.printf("BP %3.2f\n", sensorData.BME680_Press_hPa);
    Log.printf("BR %3.2f\n", sensorData.BME680_kOhms);
    Log.printf("V1 %3.2f\n", sensorData.BAT1_Volts);
    Log.printf("V2 %3.2f\n", sensorData.BAT2_Volts);
}

void switchOverTo_ECS_measurments()
{
    static uint32_t next = millis() + 10000;

    while (1)
    {
        if (millis() > next)
        {
            next = millis() + 10000;
            g_EC_Meas_State = update_EC_Meas_State(g_EC_Meas_State);

            if ((g_EC_Meas_State == EC_BASELINE) || 
            (g_EC_Meas_State == EC_EXPOSURE) || 
            (g_EC_Meas_State == EC_CLEARING))
                perform_EC_Meas();             
        }
        delay(100);

    } // End While
}

void genPublishStrings()
{
    msgStr1[0] = msgStr2[0] = msgStr3[0] = '\0';

    snprintf(msgStr1, sizeof(msgStr1), "{\
    \"1\":\"%ld\",\
    \"2\":\"%ld\",\
    \"3\":\"%ld\",\
    \"4\":\"%ld\",\
    \"5\":\"%f\",\
    \"6\":\"%ld\",\
    \"7\":\"%d\",\
    \"8\":\"%d\"\
    }",
             sensorData.I1,
             sensorData.I2,
             sensorData.I3,
             sensorData.I4,
             sensorData.EC_Sens,
             sensorData.measNum,
             sensorData.VG_V,
             g_Cartridge_Num);

    snprintf(msgStr2, sizeof(msgStr2), "{\
    \"1\":\"%ld\",\
    \"2\":\"%ld\",\
    \"3\":\"%ld\",\
    \"4\":\"%ld\",\
    \"5\":\"%f\",\
    \"6\":\"%ld\",\
    \"7\":\"%d\",\
    \"8\":\"%d\"\
    }",
             sensorData.I5,
             sensorData.I6,
             sensorData.I7,
             sensorData.I8,
             sensorData.BAT1_Volts,
             sensorData.measNum,
             g_KitNum,
             g_Cartridge_Num);

    snprintf(msgStr3, sizeof(msgStr3), "{\
    \"1\":\"%f\",\
    \"2\":\"%f\",\
    \"3\":\"%d\",\
    \"4\":\"%f\",\
    \"5\":\"%f\",\
    \"6\":\"%ld\",\
    \"7\":\"%d\",\
    \"8\":\"%d\"\
    }",
             sensorData.BME680_Temp,
             sensorData.BME680_Hum,
             g_FW_Num,
             sensorData.BME680_kOhms,
             sensorData.BAT2_Volts,
             sensorData.measNum,
             sensorData.ValvesState,
             g_Cartridge_Num);

    Serial.print("~~~");
    Serial.print("strlen(msgStr1): ");
    Serial.println(strlen(msgStr1));
    Serial.println(msgStr1);
    Serial.print("strlen(msgStr2): ");
    Serial.println(strlen(msgStr2));
    Serial.println(msgStr2);
    Serial.print("strlen(msgStr3): ");
    Serial.println(strlen(msgStr3));
    Serial.println(msgStr3);
}

void publishDataToCloud()
{
    genPublishStrings();

    Particle.publish(Ch1Str, msgStr1, PRIVATE);
    Particle.publish(Ch2Str, msgStr2, PRIVATE);
    Particle.publish(Ch3Str, msgStr3, PRIVATE);
}

void triggerSystemSleepMins(uint8_t minsToSleep)
{
    // The System Power Controller ignores <= 5 pulses (in case of glitches)
    // 6 to 10 pulses sleep {1,2,5,10,15} min (useful for Debug)
    // >= 11 pulses Sleep: (Num Pulses - 10) Hrs
    // Sleep 1 hr -> 11 pulses
    // Sleep 1 week -> 177 pulses

    if (g_bPSC_Board_Fitted == false)
        return;

    uint8_t numPulses = 0;

    switch (minsToSleep)
    {
    case 1:
        numPulses = 6;
        break;
    case 2:
        numPulses = 7;
        break;
    case 5:
        numPulses = 8;
        break;
    case 10:
        numPulses = 9;
        break;
    case 15:
        numPulses = 10;
        break;
    default:
        Serial.print("triggerSystemSleepMins() invalid input value: ");
        Serial.print(minsToSleep);
        Serial.println("Allowed values are: {1,2,5,10,15} mins");
        numPulses = 0;
    }

    for (uint8_t i = 0; i < numPulses; i++)
    {
        MCP_set_low(mcp_nSLEEP_CTL);
        delay(25);
        MCP_set_high(mcp_nSLEEP_CTL);
        delay(25);
    }
}

void triggerSystemSleepHrs(uint8_t hrsToSleep)
{
    if (g_bPSC_Board_Fitted == false)
        return;

    for (uint8_t i = 0; i < (hrsToSleep + 10); i++)
    {
        MCP_set_low(mcp_nSLEEP_CTL);
        delay(25);
        MCP_set_high(mcp_nSLEEP_CTL);
        delay(25);
    }
}

void read_FirstPowerUp_Sig_Line()
{
    // Read Power System Controller MCP_MODE input
    if (MCP_read_input(mcp_FIRST_POWER_ON) == HIGH)
    {
        bFirst_Power_Up = true;
    }
    else
    {
        bFirst_Power_Up = false;
    }

    sprintf(g_dataLine, "# Reading power shutdown controller Mod board MCP_MODE input");
    SD_WriteData(g_dataLine, g_FName, TRUE);

    Serial.print("bFirst_Power_Up: ");
    Serial.println(bFirst_Power_Up);

    sprintf(g_dataLine, "# bFirst_Power_Up: %d", bFirst_Power_Up);
    SD_WriteData(g_dataLine, g_FName, TRUE);
}

void logStartupMode(e_FW_Mode fw_Mode)
{
    switch (fw_Mode)
    {
    case START_WITH_1MCP_TEST:
        sprintf(g_dataLine, "# FW Startup Mode: START_WITH_1MCP_TEST");
        SD_WriteData(g_dataLine, g_FName, TRUE);
        break;
        // Case for PV Test skipped as the SD card is not used
    case START_WITH_ECS_LONG_TEST:
        sprintf(g_dataLine, "# FW Startup Mode: START_WITH_ECS_LONG_TEST");
        SD_WriteData(g_dataLine, g_FName, TRUE);
        break;
    case START_WITH_ECS_CONTINUOUS:
        sprintf(g_dataLine, "# FW Startup Mode: START_WITH_ECS_CONTINUOUS");
        SD_WriteData(g_dataLine, g_FName, TRUE);
        break;
    default:
        sprintf(g_dataLine, "# Error: Reached default case in logStartupMode()");
        SD_WriteData(g_dataLine, g_FName, TRUE);
    }
}

void logParameterSetValues()
{
    
#ifdef PACE_STANDARD_TEST
    sprintf(g_dataLine, "# Parameter Set Used: PACE_STANDARD");
    SD_WriteData(g_dataLine, g_FName, TRUE);
#endif

#ifdef CDT_STANDARD_TEST
    sprintf(g_dataLine, "# Parameter Set Used: CDT_STANDARD");
    SD_WriteData(g_dataLine, g_FName, TRUE);
#endif

#ifdef CDT_FAST_TEST
    sprintf(g_dataLine, "# Parameter Set Used: CDT_FAST_TEST");
    SD_WriteData(g_dataLine, g_FName, TRUE);
#endif

#ifdef CLOUD_PUBLISH_ENABLED
    sprintf(g_dataLine, "# Cloud Publish Enabled: True");
#else
    sprintf(g_dataLine, "# Cloud Publish Enabled: False");
#endif
    SD_WriteData(g_dataLine, g_FName, TRUE);

    sprintf(g_dataLine, "# Raw Parameter Values: {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d}",
            c_FET_BASELINE_COUNT,
            c_FET_EXPOSURE_COUNT,
            c_FET_MEAS_CYCLES,
            c_FET_DRYING_COUNT,
            c_EC_BASELINE_COUNT,
            c_EC_EXPOSURE_COUNT,
            c_EC_MEAS_CYCLES,
            c_EC_CLEARING_COUNT,
            c_EC_SLEEP_HRS,
            c_EC_WAIT_MINS);
    SD_WriteData(g_dataLine, g_FName, TRUE);

    sprintf(g_dataLine, "# ECS Meas Cycle Settings {First, Repeat}: {%ld, %d}", g_EC_Meas_Cycles + 1, c_EC_MEAS_CYCLES + 1);
    SD_WriteData(g_dataLine, g_FName, TRUE);
}

void logCommonDetails()
{
    sprintf(g_dataLine, "# FW: %d", g_FW_Num);
    SD_WriteData(g_dataLine, g_FName, TRUE);

    sprintf(g_dataLine, "# Kit Number: %s", g_kitNumStr);
    SD_WriteData(g_dataLine, g_FName, TRUE);

    Serial.printlnf("%s", g_dataLine);

    Serial.print("Original Cartridge ID: ");
    Serial.println(g_Cartridge_Num, DEC);
    sprintf(g_dataLine, "# Cartridge Number: %d", g_Cartridge_Num);
    SD_WriteData(g_dataLine, g_FName, TRUE);

    AT24_EEPROM_Get_Unique_ID(g_readBuf, c_READ_BUF_SIZE);
    g_dataLine[0] = 0;
    AT24_Gen_Unique_ID_Str(g_readBuf, g_dataLine, c_READ_BUF_SIZE);
    g_OLED_Line1[0] = 0;
    msgStr1[0] = 0;
    strncpy(msgStr1, "# Cartridge Unique ID: ", msgStrLen);
    SD_WriteData(msgStr1, g_FName, FALSE);
    SD_WriteData(g_dataLine, g_FName, TRUE);
    msgStr1[0] = 0;
}

void set_numECmeasCycles(e_FW_Mode fw_Mode)
{
    if (bFirst_Power_Up == false)
    {
        g_EC_Meas_Cycles = c_EC_MEAS_CYCLES;
        return;
    }

    switch (fw_Mode)
    {
    case START_WITH_ECS_LONG_TEST:
#ifdef CDT_FAST_TEST
        g_EC_Meas_Cycles = (5 - 1); // 24h / 15 min = 96 cycles (12 min baseline/ 3 min exposure)
#else
        g_EC_Meas_Cycles = (96 - 1); // 24h / 15 min = 96 cycles (12 min baseline/ 3 min exposure)
#endif
        break;
    case START_WITH_ECS_CONTINUOUS:
        g_EC_Meas_Cycles = INT32_MAX;
        break;
    default:
        g_EC_Meas_Cycles = c_EC_MEAS_CYCLES;
    }
}

void set_DefaultStartupMode(uint8_t kitNum)
{
    sprintf(g_dataLine, "# Setting Default FW Mode for kitNum: %d", kitNum);
    SD_WriteData(g_dataLine, g_FName, TRUE);

    if (
        (kitNum == 4) ||
        (kitNum == 5) ||
        (kitNum == 8) ||
        (kitNum == 9) ||
        (kitNum == 10)
        )
    {
        g_Default_FW_Mode = START_WITH_ECS_LONG_TEST;
        sprintf(g_dataLine, "# Default Startup Mode: START_WITH_ECS_LONG_TEST");
        SD_WriteData(g_dataLine, g_FName, TRUE);
    }
    else
    {
        g_Default_FW_Mode = START_WITH_1MCP_TEST;
        sprintf(g_dataLine, "# Default Startup Mode: START_WITH_1MCP_TEST");
        SD_WriteData(g_dataLine, g_FName, TRUE);
    }
}

void set_ECS_VOC_ETH_Alternation_Mode(uint8_t kitNum)
{
    sprintf(g_dataLine, "# Setting ECS VOC/ ETH Alternation Mode for kitNum: %d", kitNum);
    SD_WriteData(g_dataLine, g_FName, TRUE);

    if (
        (kitNum == 4) ||
        (kitNum == 5) ||
        (kitNum > 7)
        )
    {
        g_bECS_Alt_VOC_ETH = true;
        sprintf(g_dataLine, "# Alternate ETH / VOC: True");
        SD_WriteData(g_dataLine, g_FName, TRUE);
    }
    else
    {
        g_bECS_Alt_VOC_ETH = false;
        sprintf(g_dataLine, "# Alternate ETH / VOC: False");
        SD_WriteData(g_dataLine, g_FName, TRUE);
    }
}

