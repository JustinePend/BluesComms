#ifndef TopLevel_MCP23017_h
#define TopLevel_MCP23017_h

#include <stdint.h>
#include "MCP23017-RK.h"

//MCP23017 Pins
//GPA
static const uint16_t mcp_STAT_BAT1 = 0;
static const uint16_t mcp_STAT_BAT2 = 1;
static const uint16_t mcp_nSLEEP_CTL = 2;
static const uint16_t mcp_FIRST_POWER_ON = 3;
static const uint16_t mcp_nSHDN_5V = 4;
static const uint16_t mcp_nSHDN_7V = 5;
static const uint16_t mcp_EXT_STATUS_SW = 6; 
static const uint16_t mcp_nRTC_INT = 7; 

//GPB
static const uint16_t mcp_POWER_SW = 8;
static const uint16_t mcp_T_RH_PWR_EN = 9;
static const uint16_t mcp_PV_PWR_EN = 10;
static const uint16_t mcp_EC_PWR_EN = 11;
static const uint16_t mcp_AD7790_nCS = 12;
static const uint16_t mcp_EXT_LEDS_EN = 13;
static const uint16_t mcp_FRAM_nCS = 14;
static const uint16_t mcp_SHT_nCS = 15;

//extern MCP23017 port_expander;

void MCP23017_setup(bool bPSC_BoardFitted);
void MCP_set_low(uint16_t pin);
void MCP_set_high(uint16_t pin);
int32_t MCP_read_input(uint16_t pin);

#endif  // TopLevel_MCP23017_h