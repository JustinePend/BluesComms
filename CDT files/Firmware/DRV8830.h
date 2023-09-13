/*
 * DRV8830.h
 *
 *  Created on: 21 Nov 2019
 *      Author: ee
 */

#ifndef DRV8830_H_
#define DRV8830_H_

#define PUMP_ADDR 0x60
#define VALVE_1_ADDR 0x61
#define VALVE_2_ADDR 0x62
#define VALVE_3_ADDR 0x63

#define DRV8830_CONTROL_REG              0x00
#define DRV8830_FAULT_REG              0x01

typedef enum {
   HiZ = 0,
   REV = 1,
   PORTA = 1,
   FWD = 2,
   PORTB = 2,
   BRAKE = 3
} enDriverOutConf;

void DRV8830_Init();
void DRV8830_Set_Pump(uint16_t Vout_mV, enDriverOutConf dir);
void DRV8830_Set_Valve(uint8_t valve_num, enDriverOutConf dir);
void DRV8830_Set_All_Valves(enDriverOutConf valve1_dir, enDriverOutConf valve2_dir, enDriverOutConf valve3_dir);
void DRV8830_Set_All_Valves_Twice(enDriverOutConf valve1_dir, enDriverOutConf valve2_dir, enDriverOutConf valve3_dir);
void DRV8830_Audible_Valve_Test(uint16_t pumpDriveLevel_mV);
void DRV8830_test_Pump_Valve_Board();
uint8_t DRV8830_read_Valves_Config();


#endif /* DRV8830_H_ */
