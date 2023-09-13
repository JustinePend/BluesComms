#include "Particle.h"
#include "EC_Sensor.h"
#include <stdint.h>
#include "AD7790.h"
#include "math.h" // pow()

static const uint16_t ui16sensorRange = 2000; //value is in units (PPM)
static const uint16_t ui16sensitivity = 65;	 //value is in units (nA/ppm)

uint16_t ECS_Read_Data();
float ECS_Calc_ppm();

void ECS_Init()
{
    AD7790.AD7790_SPI_Configuration();
    AD7790.writeAd7790(AD7790_RESET, 0xFF); //Resets the part for initial use
    delay(1000);
    AD7790.writeAd7790(AD7790_MODE_WRITE, 0x00);   //Mode register value (single conversion, +/- Vref input, unbuffered mode)
    AD7790.writeAd7790(AD7790_FILTER_WRITE, 0x07); // Filter register value (clock not divided down, 9.5 Hz update rate)
}

uint16_t ECS_Read_Data()
{
    uint16_t ui16Adcdata;
    do
    {
        ui16Adcdata = AD7790.readAd7790(AD7790_STATUS_READ);
    } while (ui16Adcdata & 0x80);

    ui16Adcdata = AD7790.readAd7790(AD7790_DATA_READ);

    return (ui16Adcdata);
}

float ECS_Calc_ppm(uint16_t ui16Adcdata)
{
    float fAdcVoltage = 0;
    float fConcentration = 0;

    Serial.print("# ");
    Serial.print(ui16Adcdata);
    Serial.print("\t");
    fAdcVoltage = ((ui16Adcdata / pow(2, 15)) - 1) * 1.2; // Formula for input voltage using bipolar configuration
    Serial.print(fAdcVoltage,8);
    Serial.print("\t");
    fConcentration = (fabs(fAdcVoltage) / (485 * (20000 / 1024))) / (ui16sensitivity * pow(10, -9));
    //fConcentration = (abs(fAdcVoltage) / 9472.7) / (65e-9);
    Serial.print("\t");
    Serial.print(fConcentration,8);

    return(fConcentration);
}

float ECS_Get_Reading()
{
    uint16_t ui16Adcdata = 0;
    float fConcentration = 1.0;

    ui16Adcdata = ECS_Read_Data();
    fConcentration = ECS_Calc_ppm(ui16Adcdata);

    Serial.print("\t");
    Serial.print(ui16Adcdata);
    Serial.print("\t");
    Serial.println(fConcentration,8);

    return(fConcentration);
}