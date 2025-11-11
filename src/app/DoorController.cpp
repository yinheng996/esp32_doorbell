#include "DoorController.h"
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer* s_server = nullptr;
static DoorController* s_doorController = nullptr;

DoorController::DoorController(int doorPin, const char* doorName, FnWithinHours fnWithinHours)
  : doorPin_(doorPin), doorStatus_(0), doorName_(doorName), fnWithinHours_(fnWithinHours) {
}

void DoorController::begin() {
  s_doorController = this;
  
  // Initialize door pin
  pinMode(doorPin_, OUTPUT);
  digitalWrite(doorPin_, LOW);
  doorStatus_ = 0;
  
  // Initialize web server
  s_server = new WebServer(80);
  s_server->on("/request", HTTP_POST, []() {
    if (s_doorController) s_doorController->handleRequest();
  });
  s_server->begin();
  
  Serial.println(F("[DOOR] Controller initialized"));
  Serial.printf("[DOOR] Door pin: %d\n", doorPin_);
  Serial.println(F("[DOOR] Web server started on port 80"));
}

void DoorController::loop() {
  // Handle web server requests
  if (s_server) {
    s_server->handleClient();
  }
}

void DoorController::openDoor() {
  handleOpenDoor();
}

void DoorController::closeDoor() {
  handleCloseDoor();
}

int DoorController::getDoorStatus() const {
  return doorStatus_;
}

bool DoorController::isWithinWorkingHours() const {
  if (fnWithinHours_) {
    return fnWithinHours_();
  }
  return true; // Default to allowing if no function provided
}

void DoorController::handleOpenDoor() {
  digitalWrite(doorPin_, HIGH);
  doorStatus_ = 1;
  Serial.println(F("[DOOR] >>> DOOR OPENED (pin set HIGH) <<<"));
}

void DoorController::handleCloseDoor() {
  digitalWrite(doorPin_, LOW);
  doorStatus_ = 0;
  Serial.println(F("[DOOR] >>> DOOR CLOSED (pin set LOW) <<<"));
}

void DoorController::handleRequest() {
  if (!s_server) return;
  
  if (s_server->hasArg("plain") == false) {
    s_server->send(400, "application/json", 
      "{\n\t\"status\":\"error\",\n\t\"message\":\"Bad Request - No Data Received\"\n}\n");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, s_server->arg("plain"));
  String action = doc["action"];
  
  Serial.printf("[HTTP] Received action: %s\n", action.c_str());
  
  if (action == "openDoor") {
    if (isWithinWorkingHours()) {
      handleOpenDoor();
      doorStatus_ = 1;
      s_server->send(200, "application/json", "{\"status\":\"success\"}");
      Serial.println(F("[HTTP] Sent response: success"));
      delay(1000);
    } else {
      s_server->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Door can only be opened during working hours\"}");
      Serial.println(F("[HTTP] Door open denied - outside working hours"));
    }
  }
  else if (action == "closeDoor") {
    if (isWithinWorkingHours()) {
      handleCloseDoor();
      doorStatus_ = 0;
      s_server->send(200, "application/json", "{\"status\":\"success\"}");
      Serial.println(F("[HTTP] Sent response: success"));
      delay(1000);
    } else {
      s_server->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Door can only be closed during working hours\"}");
      Serial.println(F("[HTTP] Door close denied - outside working hours"));
    }
  }
  else if (action == "getDoorStatus") {
    DynamicJsonDocument resDoc(1024);
    resDoc["status"] = doorStatus_;
    String responseString;
    serializeJson(resDoc, responseString);
    s_server->send(200, "application/json", responseString);
    Serial.printf("[HTTP] Sent door status: %d\n", doorStatus_);
  }
  else {
    s_server->send(400, "application/json", 
      "{\n\t\"status\":\"error\",\n\t\"data\":{\n\t\t\"message\":\"Invalid Action\"\n\t}\n}\n");
    Serial.println(F("[HTTP] Sent response: Invalid Action"));
  }
}
