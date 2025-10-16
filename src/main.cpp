#include <Arduino.h>
#include <WiFi.h>               // <- high-level WiFi API (includes Enterprise helper)
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "config.h"        // pins & timing (tracked)
#include "credentials.h"   // SSID + secrets (gitignored)

WiFiClientSecure tls;
HTTPClient https;

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

  tls.setInsecure();  // OK for first tests; pin CA later
  if (!https.begin(tls, url)) return false;
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int code = https.POST(body);
  String resp = https.getString();
  https.end();

  Serial.printf("[TG] HTTP %d\n", code);
  if (code != 200) Serial.println(resp);
  return code == 200;
}

static void connectEnterprise() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);

  String hn = makeHostname(DOOR_NAME);
  WiFi.setHostname(hn.c_str());

  Serial.begin(115200);
  Serial.printf("Connecting to %s (PEAP/MSCHAPv2)...\n", WIFI_SSID);

  // HIGH-LEVEL Enterprise helper (PEAP + MSCHAPv2)
  // Signature: WiFi.begin(ssid, WPA2_AUTH_PEAP, identity, username, password);
  WiFi.begin(WIFI_SSID, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 30000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
}

/* debounce + cooldown */
static bool lastStable = HIGH;         // with INPUT_PULLUP: HIGH = not pressed
static bool lastRead   = HIGH;
static unsigned long lastChangeMs = 0;
static unsigned long lastNotifyMs = 0;

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.print("Device label: "); Serial.println(DOOR_NAME);

  connectEnterprise();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi OK. IP: "); Serial.println(WiFi.localIP());
    sendTelegram(String("🔌 <b>") + DOOR_NAME + "</b> online");
  } else {
    Serial.println("Enterprise Wi-Fi connect FAILED");
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
          String msg = String("🔔 <b>") + DOOR_NAME + "</b> doorbell pressed";
          bool ok = sendTelegram(msg);
          Serial.println(ok ? "TG sent" : "TG send failed");
        } else {
          Serial.println("Pressed (cooldown) -> not sending");
        }
      }
    }
  }
  delay(5);
}
