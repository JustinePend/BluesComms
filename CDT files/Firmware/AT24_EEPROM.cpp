#include "AT24_EEPROM.h"
#include "TopLevel_TCA9548.h"
#include <Wire.h>
#include <stdint.h> // uint8_t
#include <stdio.h> // snprintf()

char AT24_EEPROM_Get_Cartridge_Number()
{
    char ch = 0;

    TCA9548_setChannel(m_EEPROM);
    Wire.beginTransmission(EEPROM_I2C_ADDR);
    Wire.write(0);
    Wire.endTransmission();

    Wire.requestFrom(WireTransmission(EEPROM_I2C_ADDR).quantity(1).timeout(100ms));

    while (Wire.available())
    {                     // slave may send less than requested
        ch = Wire.read(); // receive a byte as character
    }

    return (ch);
}

void AT24_EEPROM_Get_Unique_ID(char *buf, const size_t maxLen)
{

    uint8_t j = 0;
    char ch;

    TCA9548_setChannel(m_EEPROM);
    Wire.beginTransmission(EEPROM_UNIQ_ID_I2C_ADDR);
    Wire.write(UNIQ_ID_WORD_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(WireTransmission(EEPROM_UNIQ_ID_I2C_ADDR).quantity(16).timeout(100ms));

    while (Wire.available())
    {                     // slave may send less than requested
        ch = Wire.read(); // receive a byte as character
        if (j < maxLen)
        {
            buf[j] = ch;
            j++;
        }
    }
    buf[j] = '\0';
}

void AT24_Gen_Unique_ID_Str(char *charArrayIn, char *charArrayOut, const size_t maxLen)
{
    char hexString[3] = "";

    charArrayOut[0] = '\0';
    uint8_t j = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        snprintf(hexString, 3, "%02X", charArrayIn[i]);
        snprintf(charArrayOut, maxLen, "%s%s", charArrayOut, hexString);
        j++;
        if (j > 1)
        {
            j = 0;
            snprintf(charArrayOut, maxLen, "%s ", charArrayOut);
        }
    }
}