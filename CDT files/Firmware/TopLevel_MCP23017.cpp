#include "Particle.h"
#include "TopLevel_MCP23017.h"

MCP23017 port_expander(Wire, 0); // Addr 0

void MCP23017_setup(bool bPSC_BoardFitted){
    // MCP23017 pins default to input on power on
    port_expander.begin();
	port_expander.pinMode(mcp_EC_PWR_EN, OUTPUT);
	port_expander.digitalWrite(mcp_EC_PWR_EN, HIGH);
	
	port_expander.pinMode(mcp_AD7790_nCS, OUTPUT);
	port_expander.digitalWrite(mcp_AD7790_nCS, HIGH);

	port_expander.pinMode(mcp_EXT_LEDS_EN, OUTPUT);
	port_expander.digitalWrite(mcp_EXT_LEDS_EN, HIGH);

    port_expander.pinMode(mcp_PV_PWR_EN, OUTPUT);
    port_expander.digitalWrite(mcp_PV_PWR_EN, HIGH);
	
    port_expander.pinMode(mcp_T_RH_PWR_EN, OUTPUT);
    port_expander.digitalWrite(mcp_T_RH_PWR_EN, HIGH);

    port_expander.digitalWrite(mcp_nSHDN_7V, LOW);
    port_expander.pinMode(mcp_nSHDN_7V, OUTPUT);

    port_expander.pinMode(mcp_EXT_STATUS_SW, INPUT);

    // If the Power Shutdown Controller (SMPS Mod Board)
    // is not fitted nSLEEP_CTL is still attached to CTL_BAT1
    // in that case we want to avoid driving that line
    if(bPSC_BoardFitted)
    {
        port_expander.pinMode(mcp_FIRST_POWER_ON, INPUT);

        port_expander.digitalWrite(mcp_nSLEEP_CTL, HIGH);
        port_expander.pinMode(mcp_nSLEEP_CTL, OUTPUT);
    }
}

void MCP_set_low(uint16_t pin){
    port_expander.digitalWrite(pin, LOW);
}

void MCP_set_high(uint16_t pin){
    port_expander.digitalWrite(pin, HIGH);
}

int32_t MCP_read_input(uint16_t pin)
{
    return(port_expander.digitalRead(pin));
}

