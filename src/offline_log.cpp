#include "offline_log.h"
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "credentials.h"   

// ---- Settings
static const char* kLogPath   = "/offline_presses.log";
static const size_t kRotateKB = 32;         // rotate at ~32 KB
static const bool   kSendLocalTimes = true; // format HH:MM SGT if time is valid

namespace {

bool fsReady = false;

String urlencode(const String& s) {
  String out; out.reserve(s.length() * 3);
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); ++i) {
    uint8_t c = (uint8_t)s[i];
    if (isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~'||c==' ') {
      out += (c==' ') ? '+' : (char)c;
    } else {
      out += '%'; out += hex[c>>4]; out += hex[c&15];
    }
  }
  return out;
}

String fmtHHMM(time_t t) {
  if (t <= 0) return String("unknown");
  struct tm tmLocal{};
  localtime_r(&t, &tmLocal);
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", tmLocal.tm_hour, tmLocal.tm_min);
  return String(buf);
}

bool sendTelegramText(const String& text) {
  WiFiClientSecure client;
  client.setInsecure(); // TODO: pin CA for production
  const String host = "api.telegram.org";
  const int    port = 443;
  if (!client.connect(host.c_str(), port)) return false;

  String body = "chat_id=" + String(TG_CHAT_ID) + "&text=" + urlencode(text);
  String req  = "POST /bot" + String(TG_BOT_TOKEN) + "/sendMessage HTTP/1.1\r\n";
  req += "Host: " + host + "\r\n";
  req += "Content-Type: application/x-www-form-urlencoded\r\n";
  req += "Content-Length: " + String(body.length()) + "\r\n";
  req += "Connection: close\r\n\r\n";
  req += body;

  client.print(req);

  // Crude status check
  unsigned long start = millis();
  bool ok = false;
  while (client.connected() && millis() - start < 6000) {
    String line = client.readStringUntil('\n');
    if (line.startsWith("HTTP/1.1 200")) { ok = true; break; }
    if (line == "\r") break;
  }
  return ok;
}

} // namespace

namespace offlinelog {

void begin() {
  if (fsReady) return;
  // Try mount; if it fails, format then mount again
  if (!LittleFS.begin(true)) {   // <-- 'true' = format on fail
    Serial.println("[offline_log] LittleFS mount+format failed");
    fsReady = false;
    return;
  }
  fsReady = true;
  Serial.println("[offline_log] LittleFS mounted");
}


void logPress(uint32_t epochSeconds) {
  if (!fsReady) return;
  File f = LittleFS.open(kLogPath, FILE_APPEND);
  if (!f) { Serial.println("[offline_log] open append failed"); return; }

  // Compact JSON line: {"door":"<name>","t":1739743321}
  f.printf("{\"door\":\"%s\",\"t\":%lu}\n", DOOR_NAME, (unsigned long)epochSeconds);
  f.close();

  // Simple rotation
  File r = LittleFS.open(kLogPath, FILE_READ);
  if (r) {
    size_t sz = r.size();
    r.close();
    if (sz > kRotateKB * 1024UL) {
      LittleFS.remove("/offline_presses.1");
      LittleFS.rename(kLogPath, "/offline_presses.1");
      Serial.println("[offline_log] rotated log");
    }
  }
}

bool reportAndClear() {
  if (!fsReady) return true;
  if (!LittleFS.exists(kLogPath)) return true;

  File f = LittleFS.open(kLogPath, FILE_READ);
  if (!f) return false;

  String msg;
  msg.reserve(1024);
  msg += String(DOOR_NAME) + " — queued presses (outside hours):\n";

  bool any = false;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (line.length() == 0) break;

    // minimal parse for "t":<num>
    int ti = line.indexOf("\"t\":");
    time_t t = 0;
    if (ti >= 0) { ti += 4; t = (time_t) strtoul(line.c_str() + ti, nullptr, 10); }

    if (kSendLocalTimes && t > 0) {
      msg += " • " + fmtHHMM(t) + "\n";
    } else {
      msg += " • epoch " + String((unsigned long)t) + "\n";
    }
    any = true;
    if (msg.length() > 3000) { msg += " …"; break; } // Telegram safety
  }
  f.close();

  if (!any) { LittleFS.remove(kLogPath); return true; }

  bool ok = sendTelegramText(msg);
  if (ok) {
    LittleFS.remove(kLogPath);
    Serial.println("[offline_log] reported and cleared");
  } else {
    Serial.println("[offline_log] report failed; keeping log");
  }
  return ok;
}

void clearNow() {
  if (!fsReady) return;
  if (LittleFS.exists(kLogPath)) LittleFS.remove(kLogPath);
}

} // namespace offlinelog