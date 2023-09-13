/*
  AD7790.h - 
*/
#ifndef AD7790_h
#define AD7790_h

//#include <Arduino.h>

/*****************************************************************************/
/************************** ADC Address Definitions **************************/
/*****************************************************************************/

//ADC Write Commands
#define AD7790_MODE_WRITE			0x10					// Write to the Mode Register
#define AD7790_FILTER_WRITE		0x20					// Write to the Filter Register

//ADC Read Commands
#define AD7790_STATUS_READ			0x08					// Read from the Status Register
#define AD7790_MODE_READ			0x18					// Read from the Mode Register
#define AD7790_FILTER_READ			0x28					// Read from the Filter Register
#define AD7790_DATA_READ			0x38					// Read from the Data Register

#define AD7790_RESET				0xFF					// Resets the chip to default

//Pins

class AD7790class
{
	public:
			void writeAd7790 (uint8_t ui8address, uint8_t ui8value);
			uint16_t readAd7790 (uint8_t ui8address);
			void AD7790_SPI_Configuration(void);
	private:

};

extern AD7790class AD7790;

#endif
