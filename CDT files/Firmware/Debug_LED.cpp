#include "Particle.h"
#include "Debug_LED.h"

int led1 = D7;
bool ledLit = false;

void DBG_DebugLED_INIT(){
    pinMode(led1, OUTPUT);
}

void DBG_ToggleDebugLED(){
    if(ledLit){
        ledLit = false;
        digitalWrite(led1, LOW);
        }
    else{ 
        ledLit= true;
        digitalWrite(led1, HIGH);
    }
}

void DBG_PulseDebugLED(uint8_t high_ms, uint8_t low_ms){
    digitalWrite(led1, HIGH);
    delay(high_ms);
    digitalWrite(led1, LOW);
    delay(low_ms);
}

void DBG_LED_SetHigh(){
    digitalWrite(led1, HIGH);
}

void DBG_LED_SetLow(){
    digitalWrite(led1, LOW);
}