#include "Notifier.h"
#include "DoorController.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <IPAddress.h>

static WiFiClientSecure s_botClient;
static WiFiClientSecure s_tls;
static WiFiClientSecure s_callbackClient;
static HTTPClient       s_https;
static HTTPClient       s_callbackHttp;
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

bool Notifier::sendOnline()  { return sendTelegram_(String("<b>") + door_ + "</b> online", true); }
bool Notifier::sendOffline() { return sendTelegram_(String("<b>") + door_ + "</b> offline", true); }
bool Notifier::sendPressed() { 
  // Professional, minimal doorbell notification
  String message = String("🔔 <b>") + String(door_) + String("</b>\n") +
                   String("Doorbell pressed");
  // Inline keyboard with clean buttons
  String keyboardJson = "{\"inline_keyboard\":[[{\"text\":\"Open\",\"callback_data\":\"open_door\"},{\"text\":\"Close\",\"callback_data\":\"close_door\"}],[{\"text\":\"Status\",\"callback_data\":\"check_status\"}]]}";
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
        String msg = String("🔴 <b>") + String(doorController_->getDoorName()) + "</b> opened";
        sendTelegramToChat_(chat_id, msg, true);
        Serial.printf("[TG] Sent response: %s\n", msg.c_str());
      } else {
        String msg = String("Outside working hours");
        sendTelegramToChat_(chat_id, msg);
        Serial.println(F("[TG] Door open denied - outside working hours"));
      }
    }
    else if (text == "/close_door") {
      if (doorController_->isWithinWorkingHours()) {
        doorController_->closeDoor();
        String msg = String("🟢 <b>") + String(doorController_->getDoorName()) + "</b> closed";
        sendTelegramToChat_(chat_id, msg, true);
        Serial.printf("[TG] Sent response: %s\n", msg.c_str());
      } else {
        String msg = String("Outside working hours");
        sendTelegramToChat_(chat_id, msg);
        Serial.println(F("[TG] Door close denied - outside working hours"));
      }
    }
    else if (text == "/read_status") {
      int status = doorController_->getDoorStatus();
      String statusMsg = String("🟡 <b>") + String(doorController_->getDoorName()) + String("</b>\n") + 
                         String(status ? "Open" : "Closed");
      sendTelegramToChat_(chat_id, statusMsg, true);
      Serial.printf("[TG] Sent response: %s\n", statusMsg.c_str());
    }
    else {
      sendTelegramToChat_(chat_id, "Unknown command");
      Serial.println(F("[TG] Sent response: Unknown command!"));
    }
  }
}

void Notifier::handleCallbackQuery(String queryId, String queryData, String chatId) {
  if (!doorController_) return;
  
  Serial.printf("[TG] Callback query received: %s\n", queryData.c_str());
  
  if (queryData == "open_door") {
    if (doorController_->isWithinWorkingHours()) {
      doorController_->openDoor();
      String msg = String("🔴 <b>") + String(doorController_->getDoorName()) + "</b> opened";
      sendTelegramToChat_(chatId, msg, true);
      Serial.printf("[TG] Sent response: %s\n", msg.c_str());
    } else {
      String msg = String("Outside working hours");
      sendTelegramToChat_(chatId, msg);
      Serial.println(F("[TG] Door open denied - outside working hours"));
    }
  }
  else if (queryData == "close_door") {
    if (doorController_->isWithinWorkingHours()) {
      doorController_->closeDoor();
      String msg = String("🟢 <b>") + String(doorController_->getDoorName()) + "</b> closed";
      sendTelegramToChat_(chatId, msg, true);
      Serial.printf("[TG] Sent response: %s\n", msg.c_str());
    } else {
      String msg = String("Outside working hours");
      sendTelegramToChat_(chatId, msg);
      Serial.println(F("[TG] Door close denied - outside working hours"));
    }
  }
  else if (queryData == "check_status") {
    int status = doorController_->getDoorStatus();
    String statusMsg = String("🟡 <b>") + String(doorController_->getDoorName()) + String("</b>\n") + 
                       String(status ? "Open" : "Closed");
    sendTelegramToChat_(chatId, statusMsg, true);
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
  
  // Ensure previous connection is fully closed
  s_callbackHttp.end();
  s_callbackClient.stop();
  delay(10); // Small delay to allow socket cleanup
  
  String url = String("https://api.telegram.org/bot") + bot_ + "/answerCallbackQuery";
  String body = "callback_query_id=" + queryId;
  
  s_callbackClient.setInsecure();
  s_callbackClient.setTimeout(3000);
  if (!s_callbackHttp.begin(s_callbackClient, url)) {
    Serial.println(F("[TG] Failed to begin callback query answer"));
    return;
  }
  s_callbackHttp.addHeader("Content-Type", "application/x-www-form-urlencoded");
  s_callbackHttp.setTimeout(3000);
  
  int code = s_callbackHttp.POST(body);
  if (code > 0) {
    s_callbackHttp.getString();
  }
  s_callbackHttp.end();
  s_callbackClient.stop();
  
  if (code != 200 && code > 0) {
    Serial.printf("[TG] Callback query HTTP %d\n", code);
  }
}

bool Notifier::sendTelegram_(const String& text, bool html) {
  return sendTelegramToChat_(String(chat_), text, html);
}

bool Notifier::sendTelegramToChat_(const String& chatId, const String& text, bool html) {
  if (!ensureDnsReady_()) return false;
  
  // Ensure previous connection is fully closed
  s_https.end();
  s_tls.stop();
  delay(10); // Small delay to allow socket cleanup
  
  String url  = String("https://api.telegram.org/bot") + bot_ + "/sendMessage";
  String body = "chat_id=" + chatId + "&text=" + text;
  if (html) body += "&parse_mode=HTML";

  s_tls.setInsecure();
  s_tls.setTimeout(5000);
  if (!s_https.begin(s_tls, url)) {
    Serial.println(F("[TG] Failed to begin HTTPS connection"));
    return false;
  }
  s_https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  s_https.setTimeout(5000);

  int code = s_https.POST(body);
  if (code > 0) {
    s_https.getString(); // Read response
  }
  s_https.end();
  s_tls.stop();

  if (code != 200 && code > 0) {
    Serial.printf("[TG] HTTP %d\n", code);
  }
  return code == 200;
}

bool Notifier::sendTelegramWithKeyboard_(const String& text, const String& keyboardJson) {
  if (!ensureDnsReady_()) return false;
  
  // Ensure previous connection is fully closed
  s_https.end();
  s_tls.stop();
  delay(10); // Small delay to allow socket cleanup
  
  String url  = String("https://api.telegram.org/bot") + bot_ + "/sendMessage";
  
  // URL encode the text and keyboard
  String body = "chat_id=" + String(chat_) + "&text=" + text;
  body += "&parse_mode=HTML";
  body += "&reply_markup=" + keyboardJson;

  s_tls.setInsecure();
  s_tls.setTimeout(5000);
  if (!s_https.begin(s_tls, url)) {
    Serial.println(F("[TG] Failed to begin HTTPS connection (keyboard)"));
    return false;
  }
  s_https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  s_https.setTimeout(5000);

  int code = s_https.POST(body);
  if (code > 0) {
    s_https.getString(); // Read response
  }
  s_https.end();
  s_tls.stop();

  if (code != 200 && code > 0) {
    Serial.printf("[TG] HTTP %d (keyboard)\n", code);
  }
  return code == 200;
}
