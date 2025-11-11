#include "DoorController.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

static WiFiClientSecure s_client;
static WiFiClientSecure s_callbackClient;
static HTTPClient s_callbackHttp;
static UniversalTelegramBot* s_bot = nullptr;
static WebServer* s_server = nullptr;
static DoorController* s_doorController = nullptr;

DoorController::DoorController(const char* botToken, int doorPin, const char* doorName, FnWithinHours fnWithinHours)
  : botToken_(botToken), doorPin_(doorPin), doorStatus_(0), doorName_(doorName), fnWithinHours_(fnWithinHours) {
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
  if (s_bot && WiFi.status() == WL_CONNECTED) {
    int numNewMessages = s_bot->getUpdates(s_bot->last_message_received + 1);
    if (numNewMessages > 0) {
      handleNewMessages(numNewMessages);
      delay(100); // Small delay to prevent connection conflicts
      
      // Handle callback queries (inline keyboard button presses)
      for (int i = 0; i < numNewMessages; i++) {
        if (s_bot->messages[i].type == "callback_query") {
          String queryId = s_bot->messages[i].query_id;
          String queryData = s_bot->messages[i].text;
          String chatId = String(s_bot->messages[i].chat_id);
          handleCallbackQuery(queryId, queryData, chatId);
          delay(100); // Delay before answering callback
          // Answer callback query via HTTP
          answerCallbackQuery(queryId);
          delay(100); // Delay after operations
        }
      }
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

void DoorController::handleNewMessages(int numNewMessages) {
  if (!s_bot || WiFi.status() != WL_CONNECTED) return;
  
  for (int i = 0; i < numNewMessages; i++) {
    // Skip callback queries, they're handled separately
    if (s_bot->messages[i].type == "callback_query") {
      continue;
    }
    
    String chat_id = String(s_bot->messages[i].chat_id);
    String text = s_bot->messages[i].text;
    
    Serial.printf("[TG] Received command: %s from chat_id: %s\n", text.c_str(), chat_id.c_str());
    
    if (text == "/open_door") {
      if (isWithinWorkingHours()) {
        handleOpenDoor();
        String msg = String("🔴 ") + doorName_ + " opened";
        delay(50);
        s_bot->sendMessage(chat_id, msg);
        Serial.printf("[TG] Sent response: %s\n", msg.c_str());
      } else {
        String msg = String("⏰ ") + doorName_ + " can only be opened during working hours";
        delay(50);
        s_bot->sendMessage(chat_id, msg);
        Serial.println(F("[TG] Door open denied - outside working hours"));
      }
    }
    else if (text == "/close_door") {
      if (isWithinWorkingHours()) {
        handleCloseDoor();
        String msg = String("🟢 ") + doorName_ + " closed";
        delay(50);
        s_bot->sendMessage(chat_id, msg);
        Serial.printf("[TG] Sent response: %s\n", msg.c_str());
      } else {
        String msg = String("⏰ ") + doorName_ + " can only be closed during working hours";
        delay(50);
        s_bot->sendMessage(chat_id, msg);
        Serial.println(F("[TG] Door close denied - outside working hours"));
      }
    }
    else if (text == "/read_status") {
      String statusMsg = doorStatus_ ? (String("🔴 ") + doorName_ + " is opened") : (String("🟢 ") + doorName_ + " is closed");
      delay(50);
      s_bot->sendMessage(chat_id, statusMsg);
      Serial.printf("[TG] Sent response: %s\n", statusMsg.c_str());
    }
    else {
      delay(50);
      s_bot->sendMessage(chat_id, "Unknown command!");
      Serial.println(F("[TG] Sent response: Unknown command!"));
    }
    delay(100); // Delay between processing messages
  }
}

void DoorController::answerCallbackQuery(String queryId) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Ensure previous connection is closed
  if (s_callbackHttp.connected()) {
    s_callbackHttp.end();
  }
  
  String url = String("https://api.telegram.org/bot") + botToken_ + "/answerCallbackQuery";
  String body = "callback_query_id=" + queryId;
  
  s_callbackClient.setInsecure();
  s_callbackClient.setTimeout(5000); // 5 second timeout
  if (!s_callbackHttp.begin(s_callbackClient, url)) {
    Serial.println(F("[TG] Failed to begin callback query answer"));
    return;
  }
  s_callbackHttp.addHeader("Content-Type", "application/x-www-form-urlencoded");
  s_callbackHttp.setTimeout(5000);
  
  int code = s_callbackHttp.POST(body);
  if (code > 0) {
    s_callbackHttp.getString(); // Read response to clear buffer
  }
  s_callbackHttp.end();
  s_callbackClient.stop(); // Ensure connection is fully closed
  
  if (code != 200 && code > 0) {
    Serial.printf("[TG] Failed to answer callback query: HTTP %d\n", code);
  } else if (code <= 0) {
    Serial.printf("[TG] Callback query answer error: %d\n", code);
  }
}

void DoorController::handleCallbackQuery(String queryId, String queryData, String chatId) {
  if (!s_bot) return;
  
  Serial.printf("[TG] Callback query received: %s\n", queryData.c_str());
  
  if (queryData == "open_door") {
    if (isWithinWorkingHours()) {
      handleOpenDoor();
      String msg = String("🔴 ") + doorName_ + " opened";
      s_bot->sendMessage(chatId, msg);
      Serial.printf("[TG] Sent response: %s\n", msg.c_str());
    } else {
      String msg = String("⏰ ") + doorName_ + " can only be opened during working hours";
      s_bot->sendMessage(chatId, msg);
      Serial.println(F("[TG] Door open denied - outside working hours"));
    }
  }
  else if (queryData == "close_door") {
    if (isWithinWorkingHours()) {
      handleCloseDoor();
      String msg = String("🟢 ") + doorName_ + " closed";
      s_bot->sendMessage(chatId, msg);
      Serial.printf("[TG] Sent response: %s\n", msg.c_str());
    } else {
      String msg = String("⏰ ") + doorName_ + " can only be closed during working hours";
      s_bot->sendMessage(chatId, msg);
      Serial.println(F("[TG] Door close denied - outside working hours"));
    }
  }
  else if (queryData == "check_status") {
    String statusMsg = doorStatus_ ? (String("🔴 ") + doorName_ + " is opened") : (String("🟢 ") + doorName_ + " is closed");
    s_bot->sendMessage(chatId, statusMsg);
    Serial.printf("[TG] Sent response: %s\n", statusMsg.c_str());
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

