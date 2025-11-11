#include "DoorController.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WebServer.h>
#include <ArduinoJson.h>

static WiFiClientSecure s_client;
static UniversalTelegramBot* s_bot = nullptr;
static WebServer* s_server = nullptr;
static DoorController* s_doorController = nullptr;

DoorController::DoorController(const char* botToken, int doorPin)
  : botToken_(botToken), doorPin_(doorPin), doorStatus_(0) {
}

void DoorController::begin() {
  s_doorController = this;
  
  // Initialize door pin
  pinMode(doorPin_, OUTPUT);
  digitalWrite(doorPin_, LOW);
  doorStatus_ = 0;
  
  // Initialize Telegram bot
  s_client.setInsecure();
  s_bot = new UniversalTelegramBot(botToken_, s_client);
  
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
  // Handle Telegram bot messages
  if (s_bot) {
    int numNewMessages = s_bot->getUpdates(s_bot->last_message_received + 1);
    if (numNewMessages > 0) {
      handleNewMessages(numNewMessages);
    }
  }
  
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

void DoorController::handleNewMessages(int numNewMessages) {
  if (!s_bot) return;
  
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(s_bot->messages[i].chat_id);
    String text = s_bot->messages[i].text;
    
    Serial.printf("[TG] Received command: %s from chat_id: %s\n", text.c_str(), chat_id.c_str());
    
    if (text == "/open_door") {
      handleOpenDoor();
      s_bot->sendMessage(chat_id, "Door opened!");
      Serial.println(F("[TG] Sent response: Door opened!"));
    }
    else if (text == "/close_door") {
      handleCloseDoor();
      s_bot->sendMessage(chat_id, "Door closed!");
      Serial.println(F("[TG] Sent response: Door closed!"));
    }
    else if (text == "/read_status") {
      String statusMsg = doorStatus_ ? "Door is opened!" : "Door is closed!";
      s_bot->sendMessage(chat_id, statusMsg);
      Serial.printf("[TG] Sent response: %s\n", statusMsg.c_str());
    }
    else {
      s_bot->sendMessage(chat_id, "Unknown command!");
      Serial.println(F("[TG] Sent response: Unknown command!"));
    }
  }
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
    handleOpenDoor();
    doorStatus_ = 1;
    s_server->send(200, "application/json", "{\"status\":\"success\"}");
    Serial.println(F("[HTTP] Sent response: success"));
    delay(1000);
  }
  else if (action == "closeDoor") {
    handleCloseDoor();
    doorStatus_ = 0;
    s_server->send(200, "application/json", "{\"status\":\"success\"}");
    Serial.println(F("[HTTP] Sent response: success"));
    delay(1000);
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

