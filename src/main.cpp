#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "config.h"
#include "credentials.h"
#include "schedule.h"

// --- prototypes ---
static String makeHostname(const char* label);
static bool   sendTelegram(const String& text);
static void   connectWiFiPSK();
// ---------------

WiFiClientSecure tls;
HTTPClient https;

// debounce + cooldown state
static bool lastStable = HIGH;   // with INPUT_PULLUP: HIGH = not pressed
static bool lastRead   = HIGH;
static unsigned long lastChangeMs = 0;
static unsigned long lastNotifyMs = 0;

// ========================== setup / loop ===================================
void setup() {
  Serial.begin(115200);
  delay(50);

  Serial.print("Device label: "); Serial.println(DOOR_NAME);
  pinMode(BTN_PIN, INPUT_PULLUP);

  connectWiFiPSK();

  if (WiFi.status() == WL_CONNECTED) {
    // Try NTP for ~10s, then print the local time if synced
    if (scheduleEnsureTime(10000)) {
      schedulePrintCurrentLocalTime();
    } else {
      Serial.println("[TIME] not synced yet (will still operate per policy)");
    }

    if (isWithinSchedule()) {
      sendTelegram(String("🔌 <b>") + DOOR_NAME + "</b> online");
    } else {
      Serial.println("[SCHED] Off-hours: suppressing 'online' notification");
    }
  } else {
    Serial.println("Wi-Fi connect FAILED");
  }
}

void loop() {
  int raw = digitalRead(BTN_PIN);
  unsigned long now = millis();

  if (raw != lastRead) { lastRead = raw; lastChangeMs = now; }
  if ((now - lastChangeMs) > DEBOUNCE_MS) {
    if (lastStable != lastRead) {
      lastStable = lastRead;
      if (lastStable == LOW) { // falling edge = press
        if (now - lastNotifyMs > COOLDOWN_MS) {
          lastNotifyMs = now;

          if (!isWithinSchedule()) {
            Serial.println("[SCHED] Button press ignored (off-hours)");
          } else {
            String msg = String("🔔 <b>") + DOOR_NAME + "</b> doorbell pressed";
            bool ok = sendTelegram(msg);
            Serial.println(ok ? "TG sent" : "TG send failed");
          }
        } else {
          Serial.println("Pressed (cooldown) -> not sending");
        }
      }
    }
  }
  delay(5);
}
// ===========================================================================



// ======================== Implementations ==================================
static String makeHostname(const char* label) {
  String s = "doorbell-";
  s += label;
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (c == ' ') s[i] = '-';
    else s[i] = (char)tolower(c);
  }
  return s;
}

static bool sendTelegram(const String& text) {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url  = String("https://api.telegram.org/bot") + TG_BOT_TOKEN + "/sendMessage";
  String body = "chat_id=" + String(TG_CHAT_ID) + "&text=" + text + "&parse_mode=HTML";

  tls.setInsecure();  // OK for testing; for production use setCACert/Bundle
  if (!https.begin(tls, url)) return false;
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int code = https.POST(body);
  String resp = https.getString();
  https.end();

  Serial.printf("[TG] HTTP %d\n", code);
  if (code != 200) Serial.println(resp);
  return code == 200;
}

static void connectWiFiPSK() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);

  String hn = makeHostname(DOOR_NAME);
  WiFi.setHostname(hn.c_str());

  Serial.printf("Connecting to %s (PSK)...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 30000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi OK. IP: "); Serial.println(WiFi.localIP());
  }
}
// ===========================================================================
