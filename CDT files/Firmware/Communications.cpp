#include "Particle.h"
#include "Communications.h"

void UART_Init(){
    //Serial.begin(9600);
    Serial.begin(57600);

	// Serial1 interfaces to ADuCM360
	Serial1.begin(9600);    
}

