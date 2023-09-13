#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <Notecard.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include "ArduinoJson.h"

#define COMM_CMD  ("cmd")
#define COMM_ARG  ("arg")
#define COMM_RES  ("res")
#define COMM_REQ  ("req")



int processLastCommand();
void processResponse();

int getSensorInterval();

//Overloading operator for J type
void convertJToJson(J* src, JsonDocument& dst) {
    deserializeJson(dst, JConvertToJSONString(src));
}

//Converts JSON Object to a String, which is then parsed as a CJSON object
void convertJsonToJ(JsonVariantConst src, J*& dst, J*& req) {
    dst = JParse(src.as<String>().c_str());
    JAddItemReferenceToObject(req, "body", dst);
}


#endif