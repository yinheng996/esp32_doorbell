#include "Notifier.h"
#include "DoorController.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <IPAddress.h>

static WiFiClientSecure s_tls;
static WiFiClientSecure s_botClient;
static HTTPClient       s_https;
static HTTPClient       s_callbackHttp;
static WiFiClientSecure s_callbackClient;
static UniversalTelegramBot* s_bot = nullptr;

Notifier::Notifier(const char* botToken, const char* chatId, const char* doorName)
: bot_(botToken), chat_(chatId), door_(doorName), doorController_(nullptr) {}

void Notifier::begin() {
  // Initialize Telegram bot for command handling
  s_botClient.setInsecure();
  s_bot = new UniversalTelegramBot(bot_, s_botClient);
  Serial.println(F("[TG] Bot initialized for commands"));
}

void Notifier::loop() {
  // Handle Telegram bot messages and callback queries
  // Keep this non-blocking - only process if we have messages
  if (s_bot && WiFi.status() == WL_CONNECTED) {
    int numNewMessages = s_bot->getUpdates(s_bot->last_message_received + 1);
    if (numNewMessages > 0) {
      handleNewMessages(numNewMessages);
      
      // Handle callback queries (inline keyboard button presses)
      for (int i = 0; i < numNewMessages; i++) {
        if (s_bot->messages[i].type == "callback_query") {
          String queryId = s_bot->messages[i].query_id;
          String queryData = s_bot->messages[i].text;
          String chatId = String(s_bot->messages[i].chat_id);
          handleCallbackQuery(queryId, queryData, chatId);
          answerCallbackQuery(queryId);
        }
      }
    }
  }
}

void Notifier::setDoorController(DoorController* doorCtrl) {
  doorController_ = doorCtrl;
}

bool Notifier::sendOnline()  { return sendTelegram_(String("🔌 <b>") + door_ + "</b> online", true); }
bool Notifier::sendOffline() { return sendTelegram_(String("🔕 <b>") + door_ + "</b> offline", true); }
bool Notifier::sendPressed() { 
  // Make the doorbell notification very prominent with formatting
  String message = String("🔔🔔🔔\n") +
                   String("<b><u>") + String(door_) + String(" DOORBELL PRESSED</u></b>\n") +
                   String("🔔🔔🔔\n\n") +
                   String("Someone is at the door!");
  // Inline keyboard with smaller, less prominent buttons
  String keyboardJson = "{\"inline_keyboard\":[[{\"text\":\"🚪 Open\",\"callback_data\":\"open_door\"},{\"text\":\"🔒 Close\",\"callback_data\":\"close_door\"}],[{\"text\":\"📊 Status\",\"callback_data\":\"check_status\"}]]}";
  return sendTelegramWithKeyboard_(message, keyboardJson);
}
bool Notifier::sendSummary(const String& text) { return sendTelegram_(text, false); }

void Notifier::handleNewMessages(int numNewMessages) {
  if (!s_bot || !doorController_ || WiFi.status() != WL_CONNECTED) return;
  
  for (int i = 0; i < numNewMessages; i++) {
    // Skip callback queries, they're handled separately
    if (s_bot->messages[i].type == "callback_query") {
      continue;
    }
    
    String chat_id = String(s_bot->messages[i].chat_id);
    String text = s_bot->messages[i].text;
    
    Serial.printf("[TG] Received command: %s from chat_id: %s\n", text.c_str(), chat_id.c_str());
    
    if (text == "/open_door") {
      if (doorController_->isWithinWorkingHours()) {
        doorController_->openDoor();
        String msg = String("🔴 ") + String(doorController_->getDoorName()) + " opened";
        delay(50);
        sendTelegramToChat_(chat_id, msg);
        Serial.printf("[TG] Sent response: %s\n", msg.c_str());
      } else {
        String msg = String("⏰ ") + String(doorController_->getDoorName()) + " can only be opened during working hours";
        delay(50);
        sendTelegramToChat_(chat_id, msg);
        Serial.println(F("[TG] Door open denied - outside working hours"));
      }
    }
    else if (text == "/close_door") {
      if (doorController_->isWithinWorkingHours()) {
        doorController_->closeDoor();
        String msg = String("🟢 ") + String(doorController_->getDoorName()) + " closed";
        delay(50);
        sendTelegramToChat_(chat_id, msg);
        Serial.printf("[TG] Sent response: %s\n", msg.c_str());
      } else {
        String msg = String("⏰ ") + String(doorController_->getDoorName()) + " can only be closed during working hours";
        delay(50);
        sendTelegramToChat_(chat_id, msg);
        Serial.println(F("[TG] Door close denied - outside working hours"));
      }
    }
    else if (text == "/read_status") {
      int status = doorController_->getDoorStatus();
      String statusMsg = String("🟡 ") + String(doorController_->getDoorName()) + (status ? " is opened" : " is closed");
      delay(50);
      sendTelegramToChat_(chat_id, statusMsg);
      Serial.printf("[TG] Sent response: %s\n", statusMsg.c_str());
    }
    else {
      delay(50);
      sendTelegramToChat_(chat_id, "Unknown command!");
      Serial.println(F("[TG] Sent response: Unknown command!"));
    }
    delay(100);
  }
}

void Notifier::handleCallbackQuery(String queryId, String queryData, String chatId) {
  if (!doorController_) return;
  
  Serial.printf("[TG] Callback query received: %s\n", queryData.c_str());
  
  if (queryData == "open_door") {
    if (doorController_->isWithinWorkingHours()) {
      doorController_->openDoor();
      String msg = String("🔴 ") + String(doorController_->getDoorName()) + " opened";
      delay(50);
      sendTelegramToChat_(chatId, msg);
      Serial.printf("[TG] Sent response: %s\n", msg.c_str());
    } else {
      String msg = String("⏰ ") + String(doorController_->getDoorName()) + " can only be opened during working hours";
      delay(50);
      sendTelegramToChat_(chatId, msg);
      Serial.println(F("[TG] Door open denied - outside working hours"));
    }
  }
  else if (queryData == "close_door") {
    if (doorController_->isWithinWorkingHours()) {
      doorController_->closeDoor();
      String msg = String("🟢 ") + String(doorController_->getDoorName()) + " closed";
      delay(50);
      sendTelegramToChat_(chatId, msg);
      Serial.printf("[TG] Sent response: %s\n", msg.c_str());
    } else {
      String msg = String("⏰ ") + String(doorController_->getDoorName()) + " can only be closed during working hours";
      delay(50);
      sendTelegramToChat_(chatId, msg);
      Serial.println(F("[TG] Door close denied - outside working hours"));
    }
  }
  else if (queryData == "check_status") {
    int status = doorController_->getDoorStatus();
    String statusMsg = String("🟡 ") + String(doorController_->getDoorName()) + (status ? " is opened" : " is closed");
    delay(50);
    sendTelegramToChat_(chatId, statusMsg);
    Serial.printf("[TG] Sent response: %s\n", statusMsg.c_str());
  }
}

bool Notifier::ensureDnsReady_() {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  IPAddress addr;
  int dnsRetries = 3;
  while (dnsRetries > 0 && WiFi.hostByName("api.telegram.org", addr) != 1) {
    Serial.println(F("[TG] DNS resolution failed, retrying..."));
    delay(1000);
    dnsRetries--;
  }
  if (dnsRetries == 0) {
    Serial.println(F("[TG] DNS resolution failed after retries"));
    return false;
  }
  return true;
}

void Notifier::answerCallbackQuery(String queryId) {
  if (!ensureDnsReady_()) return;
  
  // Ensure previous connection is closed
  if (s_callbackHttp.connected()) {
    s_callbackHttp.end();
  }
  
  String url = String("https://api.telegram.org/bot") + bot_ + "/answerCallbackQuery";
  String body = "callback_query_id=" + queryId;
  
  s_callbackClient.setInsecure();
  s_callbackClient.setTimeout(5000);
  if (!s_callbackHttp.begin(s_callbackClient, url)) {
    Serial.println(F("[TG] Failed to begin callback query answer"));
    return;
  }
  s_callbackHttp.addHeader("Content-Type", "application/x-www-form-urlencoded");
  s_callbackHttp.setTimeout(5000);
  
  int code = s_callbackHttp.POST(body);
  if (code > 0) {
    s_callbackHttp.getString();
  }
  s_callbackHttp.end();
  s_callbackClient.stop();
  
  if (code != 200 && code > 0) {
    Serial.printf("[TG] Failed to answer callback query: HTTP %d\n", code);
  } else if (code <= 0) {
    Serial.printf("[TG] Callback query answer error: %d\n", code);
  }
}

bool Notifier::sendTelegram_(const String& text, bool html) {
  return sendTelegramToChat_(String(chat_), text, html);
}

bool Notifier::sendTelegramToChat_(const String& chatId, const String& text, bool html) {
  if (!ensureDnsReady_()) return false;
  
  // Ensure previous connection is closed
  if (s_https.connected()) {
    s_https.end();
  }
  
  String url  = String("https://api.telegram.org/bot") + bot_ + "/sendMessage";
  String body = "chat_id=" + chatId + "&text=" + text;
  if (html) body += "&parse_mode=HTML";

  s_tls.setInsecure();
  s_tls.setTimeout(10000);
  if (!s_https.begin(s_tls, url)) {
    Serial.println(F("[TG] Failed to begin HTTPS connection"));
    return false;
  }
  s_https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  s_https.setTimeout(10000);

  int code = s_https.POST(body);
  String resp = s_https.getString();
  s_https.end();
  s_tls.stop();

  Serial.printf("[TG] HTTP %d\n", code);
  if (code != 200 && code > 0) Serial.println(resp);
  return code == 200;
}

bool Notifier::sendTelegramWithKeyboard_(const String& text, const String& keyboardJson) {
  if (!ensureDnsReady_()) return false;
  
  // Ensure previous connection is closed
  if (s_https.connected()) {
    s_https.end();
  }
  
  String url  = String("https://api.telegram.org/bot") + bot_ + "/sendMessage";
  
  // URL encode the text and keyboard
  String body = "chat_id=" + String(chat_) + "&text=" + text;
  body += "&parse_mode=HTML";
  body += "&reply_markup=" + keyboardJson;

  s_tls.setInsecure();
  s_tls.setTimeout(10000);
  if (!s_https.begin(s_tls, url)) {
    Serial.println(F("[TG] Failed to begin HTTPS connection (keyboard)"));
    return false;
  }
  s_https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  s_https.setTimeout(10000);

  int code = s_https.POST(body);
  String resp = s_https.getString();
  s_https.end();
  s_tls.stop();

  Serial.printf("[TG] HTTP %d (with keyboard)\n", code);
  if (code != 200 && code > 0) Serial.println(resp);
  return code == 200;
}
