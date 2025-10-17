#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <LittleFS.h>

#include "config.h"
#include "credentials.h"
#include "schedule.h"
#include "offline_log.h"

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

// for reporting when schedule flips to working hours
static bool wasWithin = false;
static unsigned long lastSchedCheck = 0;

// ========================== setup / loop ===================================
void setup() {
  Serial.begin(115200);
  delay(50);

  Serial.print("Device label: "); Serial.println(DOOR_NAME);
  pinMode(BTN_PIN, INPUT_PULLUP);

  connectWiFiPSK();

  // Start NTP (non-blocking) and try to sync briefly
  if (WiFi.status() == WL_CONNECTED) {
    if (scheduleEnsureTime(10000)) {
      schedulePrintCurrentLocalTime();
    } else {
      Serial.println("[TIME] not synced yet (will still operate per policy)");
    }
  } else {
    Serial.println("Wi-Fi connect FAILED");
  }

  // Mount LittleFS for offline logging
  offlinelog::begin();

  // Ensure localtime formatting matches schedule timezone for summaries
  setenv("TZ", SCHED_TZ, 1);
  tzset();

  // Initial “online” message only during working hours
  if (WiFi.status() == WL_CONNECTED) {
    if (isWithinSchedule()) {
      sendTelegram(String("🔌 <b>") + DOOR_NAME + "</b> online");
    } else {
      Serial.println("[SCHED] Off-hours: suppressing 'online' notification");
    }
  }
  // Initialize edge-detection memory for schedule
  wasWithin = isWithinSchedule();
}

void loop() {
  int raw = digitalRead(BTN_PIN);
  unsigned long now = millis();

  // Debounce
  if (raw != lastRead) { lastRead = raw; lastChangeMs = now; }
  if ((now - lastChangeMs) > DEBOUNCE_MS) {
    if (lastStable != lastRead) {
      lastStable = lastRead;
      if (lastStable == LOW) { // falling edge = press
        if (now - lastNotifyMs > COOLDOWN_MS) {
          lastNotifyMs = now;

          if (!isWithinSchedule()) {
            // OFF-HOURS: queue to offline log, not discarded
            uint32_t epochNow = (uint32_t)time(nullptr); // ok if 0 pre-sync
            offlinelog::logPress(epochNow);
            Serial.println("[SCHED] Off-hours: queued press in offline log");
          } else {
            // WITHIN HOURS: normal immediate notify
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

  // Check schedule once per minute; when we enter working hours, flush summary
  // Check schedule every minute (or faster during testing)
if (now - lastSchedCheck > 60UL * 1000) {
  lastSchedCheck = now;

  bool within = isWithinSchedule();

  // -> entering working hours
  if (within && !wasWithin) {
    Serial.println("[SCHED] Transition to working hours");
    if (WiFi.status() == WL_CONNECTED) {
      // (a) send ONLINE notice
      sendTelegram(String("🔌 <b>") + DOOR_NAME + "</b> online");

      // (b) flush any queued off-hours presses
      offlinelog::reportAndClear();
    } else {
      Serial.println("[SCHED] Wi-Fi down at transition; will try next minute");
    }
  }

  // -> leaving working hours (entering off-hours)
  if (!within && wasWithin) {
    Serial.println("[SCHED] Transition to off-hours");
    if (WiFi.status() == WL_CONNECTED) {
      sendTelegram(String("🔕 <b>") + DOOR_NAME + "</b> offline (outside working hours)");
    }
  }

  wasWithin = within;
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
