#ifndef AT24_EEPROM_H
#define AT24_EEPROM_H

#include <stddef.h> // size_t

#define EEPROM_UNIQ_ID_I2C_ADDR 0x58
#define EEPROM_I2C_ADDR 0x50
#define UNIQ_ID_WORD_ADDR 0x80


char AT24_EEPROM_Get_Cartridge_Number();
void AT24_EEPROM_Get_Unique_ID(char *buf, const size_t maxLen);
void AT24_Gen_Unique_ID_Str(char *charArrayIn, char *charArrayOut, const size_t maxLen);

#endif  // AT24_EEPROM_H