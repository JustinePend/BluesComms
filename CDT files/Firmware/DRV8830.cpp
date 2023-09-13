/*
 * DRV8830.c
 *
 *  Created on: 21 Nov 2019
 *      Author: ee
 */
#include "Particle.h"
#include <stdint.h>
#include "DRV8830.h"
#include <Wire.h>

// VSET
// 0x10 to 0x3FH (1.29V to 5.06V)

// Typedefs

typedef union
{
	struct
	{
		uint8_t IN1 : 1;
		uint8_t IN2 : 1;
		uint8_t VSET : 6;
	} fields;
	uint8_t val;
} control_reg_t;

typedef union
{
	struct
	{
		uint8_t valve1_state : 1;
		uint8_t valve2_state : 1;
		uint8_t valve3_state : 1;
		uint8_t : 5;
	} fields;
	uint8_t valves_overall_state;
} valve_setting_t;

typedef union
{
	struct
	{
		uint8_t FAULT : 1;
		uint8_t OCP : 1;
		uint8_t UVLO : 1;
		uint8_t OTS : 1;
		uint8_t ILIMIT : 1;
		uint8_t : 2;
		uint8_t CLEAR : 1;
	} fields;
	uint8_t val;
} fault_reg_t;

// Enums

valve_setting_t valves_setting;

/* Write data mode */
typedef enum
{
	I2C_WRITE_ONE_REG = 1, /* Write TEMP register */
	I2C_WRITE_TWO_REG,
	UART_WRITE_NO_INT, /* Write data when interrupts are disabled */
	UART_WRITE_IN_INT, /* Write data while in an interrupt routine */
	UART_WRITE
} enWriteData;

typedef enum
{
	I2C_READ_ONE_REG = 1, /* Read one TEMP register */
	I2C_READ_TWO_REG	  /* Read two TEMP registers */

} enRegsNum;

// Global Variables



// @TODO implement reading and use of DRC8830 fault codes

uint8_t DRV8830_CalcCode(uint16_t Vout_mV)
{
	uint8_t code;

	if (Vout_mV == 0)
		return (0x00);

	if (Vout_mV > 5000)
		return (0x3f);

	code = (uint8_t)((16 * Vout_mV) / 1285 + 1);
	return code;
}

void DRV8830_Init()
{
	Wire.begin();
}

void I2C_Write(uint8_t ui8deviceAddress, uint8_t ui8regAddress, uint8_t ui8Data, uint8_t ui8Data2, enWriteData enMode)
{
	//uint8_t ui8deviceAddress = ui8deviceAddress << 1;
	//int count;

	if (enMode == I2C_WRITE_ONE_REG)
	{

		//I2cTx(MASTER, ui8regAddress); /* Master sends 8-bit register address */
		Wire.beginTransmission(ui8deviceAddress);
		Wire.write(ui8regAddress);
		Wire.write(ui8Data);
		Wire.endTransmission();
	}

	else if (enMode == I2C_WRITE_TWO_REG)
	{
		Serial.println("DRV8830_I2C_Write: error A");
	}
}

void DRV8830_Set_Pump(uint16_t Vout_mV, enDriverOutConf dir)
{

	control_reg_t control_reg;

	control_reg.fields.VSET = DRV8830_CalcCode(Vout_mV);

	switch (dir)
	{
	case FWD: // Flow C to A
		control_reg.fields.IN1 = 1;
		control_reg.fields.IN2 = 0;
		break;
	case REV: // Flow C to B
		control_reg.fields.IN1 = 0;
		control_reg.fields.IN2 = 1;
		break;
	default:
		control_reg.fields.IN1 = 0;
		control_reg.fields.IN2 = 0;
	}

	I2C_Write(PUMP_ADDR, DRV8830_CONTROL_REG, control_reg.val, 0x00, I2C_WRITE_ONE_REG);
}

void DRV8830_Activate_Bistable_Valve(uint8_t ValveAddr, enDriverOutConf dir)
{
	control_reg_t control_reg;

	control_reg.fields.VSET = 0x3F; // Set Voltage To Max

	switch (dir)
	{
	case FWD:
		control_reg.fields.IN1 = 1;
		control_reg.fields.IN2 = 0;
		break;
	case REV:
		control_reg.fields.IN1 = 0;
		control_reg.fields.IN2 = 1;
		break;
	default:
		control_reg.fields.IN1 = 0;
		control_reg.fields.IN2 = 0;
	}

	I2C_Write(ValveAddr, DRV8830_CONTROL_REG, control_reg.val, 0x00, I2C_WRITE_ONE_REG);

	control_reg.fields.IN1 = 0; // IN1=0, IN2=0 Sets DRV8839 outputs to HiZ
	control_reg.fields.IN2 = 0;
	delay(60); // specified minimum delay 16 ms;

	I2C_Write(ValveAddr, DRV8830_CONTROL_REG, control_reg.val, 0x00, I2C_WRITE_ONE_REG);
}

void DRV8830_Set_Valve(uint8_t valve_num, enDriverOutConf dir)
{
	switch (valve_num)
	{
	case 1:
		DRV8830_Activate_Bistable_Valve(VALVE_1_ADDR, dir);
		(dir == PORTA) ? valves_setting.fields.valve1_state = 0 : valves_setting.fields.valve1_state = 1;		
		break;
	case 2:
		DRV8830_Activate_Bistable_Valve(VALVE_2_ADDR, dir);
		(dir == PORTA) ? valves_setting.fields.valve2_state = 0 : valves_setting.fields.valve2_state = 1;	
		break;
	case 3:
		DRV8830_Activate_Bistable_Valve(VALVE_3_ADDR, dir);
		(dir == PORTA) ? valves_setting.fields.valve3_state = 0 : valves_setting.fields.valve3_state = 1;	
		break;
	default:
		return;
	}
}

void DRV8830_Set_All_Valves(enDriverOutConf valve1_dir, enDriverOutConf valve2_dir, enDriverOutConf valve3_dir){
	delay(1000);
	DRV8830_Set_Valve(1, valve1_dir);
	delay(1000);
	DRV8830_Set_Valve(2, valve2_dir);
	delay(1000);
	DRV8830_Set_Valve(3, valve3_dir);
	delay(1000);
}

void DRV8830_Audible_Valve_Test(uint16_t pumpDriveLevel_mV)
{

	int8_t i, j;

	DRV8830_Set_Pump(0, FWD);
	delay(5000);
	for (i = 1; i < 4; i++)
	{
		DRV8830_Set_Pump(pumpDriveLevel_mV, FWD);
		delay(5000);
		DRV8830_Set_Pump(0, FWD);
		delay(1000);
		for (j = 1; j < 4; j++)
		{
			DRV8830_Set_Valve(i, PORTA);
			delay(500 + i * 200);
			DRV8830_Set_Valve(i, PORTB);
			delay(500 + i * 200);
		}
		DRV8830_Set_Pump(0, FWD);
		delay(2000);
	}
}

void DRV8830_test_Pump_Valve_Board()
{
	DRV8830_Set_Pump(5000, FWD);
	delay(1000);
	DRV8830_Set_Pump(0, FWD);
	delay(1000);
	DRV8830_Set_Pump(5000, REV);
	delay(1000);
	DRV8830_Set_Pump(0, REV);
	delay(1000);

	for (int8_t i = 1; i < 4; i++)
	{
		DRV8830_Set_Valve(i, PORTA);
		delay(1000);
		DRV8830_Set_Valve(i, PORTB);
		delay(1000);
	}
}

uint8_t DRV8830_read_Valves_Config()
{
	return(valves_setting.valves_overall_state);
}