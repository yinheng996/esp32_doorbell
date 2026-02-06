#include "Notifier.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static WiFiClientSecure s_tls;
static HTTPClient       s_https;

Notifier::Notifier(const char* botToken, const char* chatId, const char* doorName)
: bot_(botToken), chat_(chatId), door_(doorName) {}

bool Notifier::sendOnline()  { 
  String msg = String("🔌 <b>") + door_ + "</b> online\n";
  msg += "☀️ Good morning makers!";
  return sendTelegram_(msg, true); 
}
bool Notifier::sendOffline() { 
  String msg = String("🔕 <b>") + door_ + "</b> offline\n";
  msg += "🌙 Enjoy your evening!";
  return sendTelegram_(msg, true); 
}
bool Notifier::sendPressed() { return sendTelegram_(String("🔔 <b>") + door_ + "</b> rung", true); }
bool Notifier::sendPressedWithButton() {
  return sendTelegramWithButton_(
    String("🔔 <b>") + door_ + "</b> rung",
    "🔓 Release Door",
    "release_door"
  );
}
bool Notifier::sendSummary(const String& text) { return sendTelegram_(text, false); }

bool Notifier::sendReleaseConfirm(const String& userName, time_t timestamp) {
  struct tm lt{};
  localtime_r(&timestamp, &lt);
  char timeStr[32];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);
  
  String msg = String("🔓 <b>") + door_ + "</b> door released\n";
  msg += "👤 " + userName + " at " + timeStr;
  return sendTelegram_(msg, true);
}

bool Notifier::sendReleaseRejected(const String& userName, time_t timestamp) {
  struct tm lt{};
  localtime_r(&timestamp, &lt);
  char timeStr[32];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", lt.tm_hour, lt.tm_min, lt.tm_sec);
  
  String msg = String("🚫 <b>") + door_ + "</b> Rejected Door Release\n";
  msg += "👤 " + userName + " at " + timeStr;
  return sendTelegram_(msg, true);
}

bool Notifier::sendTelegram_(const String& text, bool html) {
  if (WiFi.status() != WL_CONNECTED) return false;
  String url  = String("https://api.telegram.org/bot") + bot_ + "/sendMessage";
  String body = "chat_id=" + String(chat_) + "&text=" + text;
  if (html) body += "&parse_mode=HTML";

  s_tls.setInsecure(); // TODO: setCACert for production
  if (!s_https.begin(s_tls, url)) return false;
  s_https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int code = s_https.POST(body);
  String resp = s_https.getString();
  s_https.end();

  Serial.printf("[TG] HTTP %d\n", code);
  if (code != 200) Serial.println(resp);
  return code == 200;
}

bool Notifier::sendTelegramWithButton_(const String& text, const String& buttonText, const String& callbackData) {
  if (WiFi.status() != WL_CONNECTED) return false;
  String url = String("https://api.telegram.org/bot") + bot_ + "/sendMessage";
  
  // Build inline keyboard JSON
  String keyboard = String("{\"inline_keyboard\":[[{\"text\":\"") + buttonText + 
                    "\",\"callback_data\":\"" + callbackData + "\"}]]}";
  
  String body = "chat_id=" + String(chat_) + 
                "&text=" + text + 
                "&parse_mode=HTML" +
                "&reply_markup=" + keyboard;

  s_tls.setInsecure(); // TODO: setCACert for production
  if (!s_https.begin(s_tls, url)) return false;
  s_https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int code = s_https.POST(body);
  String resp = s_https.getString();
  s_https.end();

  Serial.printf("[TG] sendWithButton HTTP %d\n", code);
  if (code != 200) Serial.println(resp);
  return code == 200;
}

void Notifier::pollUpdates(void (*onRelease)(const String&)) {
  if (WiFi.status() != WL_CONNECTED || !onRelease) return;
  
  // Throttle polling to avoid blocking main loop
  uint32_t now = millis();
  if (now - lastPollMs_ < pollIntervalMs_) return;
  lastPollMs_ = now;
  
  String url = String("https://api.telegram.org/bot") + bot_ + "/getUpdates?offset=" + 
               String(lastUpdateId_ + 1) + "&timeout=0";

  s_tls.setInsecure();
  if (!s_https.begin(s_tls, url)) return;
  
  int code = s_https.GET();
  if (code != 200) {
    s_https.end();
    return;
  }

  String payload = s_https.getString();
  s_https.end();

  // Parse JSON response
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.printf("[TG] JSON parse error: %s\n", error.c_str());
    return;
  }

  JsonArray results = doc["result"];
  for (JsonObject update : results) {
    int32_t updateId = update["update_id"];
    if (updateId > lastUpdateId_) lastUpdateId_ = updateId;

    // Check for callback_query
    if (update.containsKey("callback_query")) {
      JsonObject query = update["callback_query"];
      String callbackData = query["data"].as<String>();
      
      if (callbackData == "release_door") {
        Serial.println(F("[TG] Release door callback received"));
        
        // Extract user information
        JsonObject from = query["from"];
        String userName = "Unknown";
        if (from.containsKey("first_name")) {
          userName = from["first_name"].as<String>();
          if (from.containsKey("last_name")) {
            userName += " " + from["last_name"].as<String>();
          }
        } else if (from.containsKey("username")) {
          userName = "@" + from["username"].as<String>();
        }
        
        // Answer the callback query to remove loading state
        String queryId = query["id"].as<String>();
        String answerUrl = String("https://api.telegram.org/bot") + bot_ + 
                          "/answerCallbackQuery?callback_query_id=" + queryId;
        s_tls.setInsecure();
        if (s_https.begin(s_tls, answerUrl)) {
          s_https.GET();
          s_https.end();
        }
        
        // Trigger the release callback with user info
        onRelease(userName);
      }
    }
  }
}
