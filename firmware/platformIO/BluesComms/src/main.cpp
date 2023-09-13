
#include "main.h"

#define productUID "com.cid-inc.jpendergraft:pacedevices"

//"com.cid-inc.jpendergraft:pacetest"

Notecard notecard;

HardwareSerial SerialPort(2);

int sensorIntervalSeconds = 600;
int mode = 1;

void setup() {
  // Start serial ports
  SerialPort.begin(115200, SERIAL_8N1, 16, 17);

  Serial.begin(115200);
  Serial.println();

  delay(2500);
  notecard.begin();


  // Make a new request to connect the notecard with the hub
  // This comes in the form of a JSON object.
  // Builds a JSON object with values for product ID and mode. 
  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", productUID);
  JAddStringToObject(req, "mode", "continuous"); 
  JAddBoolToObject(req, "sync", true);

  //For periodic mode, uncomment the following (and comment out the 'mode' line above):

  // JAddStringToObject(req, "mode", "periodic");
  // JAddNumberToObject(req, "outbound", 60); //seconds for inbound sync
  // JAddNumberToObject(req, "inbound", 60); //seconds for outbound sync

  notecard.sendRequest(req);
 }

//{"cmd":"get", "arg":{"dev":"afe","index":0,"param":"data"}}

 void loop() {
  //Read from Notehub. If the command is successfully processed, then send it to main PACE board and process a response. 
  int res = processLastCommand();

  if(res) {
    delay(500); //Leave a moment for the port to become available
    processResponse();
  }

  // Retrieve the environment variable for how much to delay by. 

  sensorIntervalSeconds = getSensorInterval();
  Serial.print("Delaying ");
  Serial.print(sensorIntervalSeconds);
  Serial.println(" seconds");
  delay(sensorIntervalSeconds * 1000);
}

int processLastCommand()
{
  StaticJsonDocument<300> message;
  // Read note from sensors.qis on notehub. Delete object after finished with it. 
  J *req = notecard.newRequest("note.get");
  JAddStringToObject(req, "file", "sensors.qis");
  JAddBoolToObject(req, "delete", true);

  J *rsp = notecard.requestAndResponse(req);
  if (notecard.responseError(rsp)) {
    notecard.logDebug("No notes available");
    return 0;
  } else {
    // If sucessfully retrieved message, convert from J type to JsonDocument type
    convertJToJson(rsp, message);

    Serial.println(message.as<String>().c_str());

    //Send to PACE
    if(serializeJson(message, SerialPort) == 0) {
      Serial.println("Failed to write");
    }
  Serial.println();
  }
  notecard.deleteResponse(rsp);
  return 1;
}

void processResponse(){
  StaticJsonDocument<600> response;
  //Read the response through UART
  if(SerialPort.available()) {
    DeserializationError err = deserializeJson(response, SerialPort);
    if(err != DeserializationError::Ok) {
      // Flush all bytes in the serial port buffer if there is an error
      while (SerialPort.available() > 0)
          SerialPort.read();
    }

    Serial.print("Response: ");
    Serial.println(response.as<String>().c_str());

    // Check for valid format in response
    if (!response.containsKey(COMM_REQ) && !response.containsKey(COMM_RES)) {
      Serial.println("Invalid format");
      return;
    }

    //Write to Notehub
    //Add sensor reading to the Notecard. Construct JSON request to note.add API
    J *req = notecard.newRequest("note.add");
    if (req != NULL) 
    {
      JAddStringToObject(req, "file", "sensors.qo");
      JAddBoolToObject(req, "sync", true);
      J *body;
      if (!response.isNull()) 
      {
        // Convert from JsonDocument type to J type
        convertJsonToJ(response, body, req);
        
        //Send request to Notehub
        if(body->string != "{}") {
          Serial.print("Sending request: ");
          Serial.println(JConvertToJSONString(body));
          if(!notecard.sendRequest(req)) {
            Serial.println("Couldn't send request to notecard.");
          }
        } else {
          Serial.println("Empty body");
        }
      }
    }
  } else {
    Serial.println("Serial Port not Available");
  }
}

// This function assumes you’ll set the reading_interval environment variable to
// a positive integer. If the variable is not set, set to 0, or set to an invalid
// type, this function returns a default value of 60.
int getSensorInterval() {
  int sensorIntervalSeconds = 60;
  J *req = notecard.newRequest("env.get");
  if (req != NULL) {
      JAddStringToObject(req, "name", "interval");
      J* rsp = notecard.requestAndResponse(req);
      int readingIntervalEnvVar = atoi(JGetString(rsp, "text"));

      if (readingIntervalEnvVar > 0) {
        sensorIntervalSeconds = readingIntervalEnvVar;
      }
      notecard.deleteResponse(rsp);
  }
  return sensorIntervalSeconds;
}

