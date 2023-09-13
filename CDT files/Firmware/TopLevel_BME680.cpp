#include "TopLevel_BME680.h"
#include "Communications.h"
#include "TopLevel_TCA9548.h"

Adafruit_BME680 bme;

void BME680_Init()
{
    // Set up oversampling and filter initialization
    TCA9548_setChannel(m_BME680_Cartridge);
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms

    if (!bme.begin(0x76))
    {
        Serial.println("Could not find a valid BME680 sensor, check wiring!");
        while (1)
            ;
    }
}

void BME680_UpdateData()
{
    TCA9548_setChannel(m_BME680_Cartridge);
    if (!bme.performReading())
    {
        Serial.println("Failed to perform reading :(");
        return;
    }
    TCA9548_setChannel(m_SpareCh);
}