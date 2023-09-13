#ifndef TOPLEVEL_BME680_H
#define TOPLEVEL_BME680_H

#include "Adafruit_BME680.h"

extern Adafruit_BME680 bme;

void BME680_Init();
void BME680_UpdateData();

#endif  // TOPLEVEL_BME680_H