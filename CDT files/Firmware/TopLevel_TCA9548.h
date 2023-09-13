#ifndef TOP_LEVEL_TCA9548_H
#define TOP_LEVEL_TCA9548_H

#include "Particle.h"
#include <stdint.h>
#include "TCA9548A-RK.h"
#include <Wire.h>

extern TCA9548A mux;

typedef enum
{
    m_Pump_Valves = 0, 
    m_EEPROM, 
    m_BME680_Cartridge, 
    m_SHT31, 
    m_Status_Indicator, 
    m_BME680_Dehumid, 
    m_BME680_External, 
    m_SpareCh
}muxChs_t;

void TCA9548_Init();

void TCA9548_setChannel(muxChs_t muxCh);


#endif  // TOP_LEVEL_TCA9548_H