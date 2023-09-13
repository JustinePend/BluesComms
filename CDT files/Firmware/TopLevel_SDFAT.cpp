#include "TopLevel_SDFAT.h"
#include "Debug_LED.h"
#include <stdio.h> // snprintf()

// The SD card CS pin on the Adafruit AdaLogger FeatherWing is D5.
const int SD_CHIP_SELECT = D5;
SdFat sd;

uint32_t SD_WriteData(char* Line, char* FName, bool PrintEOL) {
    //DBG_LED_SetHigh();
	File myFile;
	// Initialize the library
	if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) {
		Serial.println("SDCARD: Failed to open card");
        //DBG_LED_SetLow();
		return(0);
	}

	// open the file for write at end like the "Native SD library"
	
	if (!myFile.open(FName, O_RDWR | O_CREAT | O_AT_END)) {
		Serial.printlnf("SDCARD: opening %s for write failed",FName);
        //DBG_LED_SetLow();
		return(0);  
	}
	// if the file opened okay, write to it:
	//Serial.printlnf("SDCARD: Writing to %s...",FName);
	//myFile.println("testing 1, 2, 3.");
	if(PrintEOL){
		myFile.println(Line);
	}
	else{
		myFile.print(Line);
	}    
	Serial.printlnf("SDCARD: fileSize (B): %d\n", myFile.fileSize());
    myFile.close();
    //DBG_LED_SetLow();
    return(myFile.fileSize());
}