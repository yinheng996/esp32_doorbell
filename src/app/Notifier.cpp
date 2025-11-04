#include "Notifier.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

static WiFiClientSecure s_tls;
static HTTPClient       s_https;

Notifier::Notifier(const char* botToken, const char* chatId, const char* doorName)
: bot_(botToken), chat_(chatId), door_(doorName) {}

bool Notifier::sendOnline()  { return sendTelegram_(String("🔌 <b>") + door_ + "</b> online", true); }
bool Notifier::sendOffline() { return sendTelegram_(String("🔕 <b>") + door_ + "</b> offline", true); }
bool Notifier::sendPressed() { return sendTelegram_(String("🔔 <b>") + door_ + "</b> doorbell pressed", true); }
bool Notifier::sendSummary(const String& text) { return sendTelegram_(text, false); }

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
