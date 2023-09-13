#include "TopLevel_RTC.h"
#include "Particle.h"
#include "RTClib.h"
#include <Wire.h>

RTC_PCF8523 rtc;

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write((byte)reg);
  Wire.write((byte)val);
  Wire.endTransmission();
}

void RTC_Init() {
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    // @TODO RTC_Init return error to main for display on OLED screen; while(1)!!!
    while (1);
  }// End If

  rtc.writeSqwPinMode(PCF8523_SquareWave1HZ);
}

void RTC_GetTime (char* Line, size_t MaxLength) {
    DateTime now = rtc.now();
    snprintf (Line, MaxLength, "%d-%02d-%02dT%02d:%02d:%02dZ",
        now.year(),
        now.month(),
        now.day(),
        now.hour(),
        now.minute(),
        now.second()
    );
    Serial.println(Line);
} // End Function

void RTC_GenDataFileName (const char* prefixStr, const char* kitNumStr, const size_t MaxLength, char* FName) {
    DateTime now = rtc.now();
    //snprintf (FName, MaxLength, "%s_DATA_Kit_%s_%d_%02d_%02d_%02dh%02dm%02d.txt",
    snprintf (FName, MaxLength, "%d_%02d_%02d_%02dh%02dm%02d_%s_DATA_Kit_%s.txt",
        now.year(),
        now.month(),
        now.day(),
        now.hour(),
        now.minute(),
        now.second(),
        prefixStr,
        kitNumStr
    );
    Serial.printlnf("RTC Generated Filename: %s",FName);
} // End Function
