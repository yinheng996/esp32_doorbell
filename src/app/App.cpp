#include "App.h"

#include "app/Net.h"
#include "app/TimeSvc.h"
#include "app/Notifier.h"
#include "app/OfflineLog.h"
#include "app/Button.h"
#include "app/Scheduler.h"
#include "app/BusinessHours.h"
#include "app/Relay.h"

#include "config.h"
#include "credentials.h"
#include "working_hours.h"

// ---------- Single instance wiring ----------
static App*        g_app = nullptr;

static Net         g_net{WIFI_SSID, WIFI_PASS, DOOR_NAME};
static TimeSvc     g_time{WORK_TZ, /*syncTimeoutMs=*/10000};
static Notifier    g_notifier{TG_BOT_TOKEN, TG_CHAT_ID, DOOR_NAME};
static OfflineLog  g_log{"/offline_presses.log", /*rotateKB=*/32};
static Button      g_btn{BTN_PIN, /*activeLow=*/true, DEBOUNCE_MS};
static BusinessHours g_hours{WORK_HOURS, WORK_TZ, WORK_ALLOW_BEFORE_SYNC, WORK_VALID_EPOCH};
static Relay       g_relay{RELAY_PIN, RELAY_ACTIVE_LOW, RELAY_PULSE_MS};

// Scheduler asks BusinessHours; we use a thunk so Scheduler can call a plain fn ptr
static bool withinThunk_() { return g_hours.withinNow(); }
static Scheduler g_sched{withinThunk_};

// ---------- App ----------
void App::begin() {
  g_app = this;

  Serial.print(F("Device: ")); Serial.println(DOOR_NAME);

  // Hardware
  g_btn.begin(&App::onPressThunk_);
  g_relay.begin();

  // Connectivity
  g_net.begin();
  g_net.waitReady(30000);

  // Time & TZ
  g_time.begin();
  g_time.printLocal();
  setenv("TZ", WORK_TZ, 1); tzset();

  // Storage
  g_log.begin(/*formatOnFail=*/true);

  // Schedule
  g_sched.begin(/*pollMs=*/1000);
  transitionHandled_ = true;

  // Initial announce only if already within working hours
  if (g_sched.within() && g_net.connected()) {
    g_notifier.sendOnline();
  } else {
    Serial.println(F("[SCHED] booted outside working hours"));
  }
}

void App::loop() {
  // Services (kept quick/non-blocking)
  g_net.loop();
  g_time.loop();
  g_btn.loop();
  g_relay.loop();

  // Poll for Telegram updates (release button callbacks)
  g_notifier.pollUpdates(&App::onReleaseThunk_);

  // Edge detection
  const auto edge = g_sched.poll(); // Scheduler::Edge
  switch (edge) {
    case Scheduler::Edge::Entered: handleEdge_(1); break;
    case Scheduler::Edge::Left:    handleEdge_(2); break;
    default: break;
  }

  // Small cooperative delay
  // (Keep the loop snappy; no long blocking here)
}

// ---------- Event plumbing ----------
void App::onPressThunk_() {
  if (g_app) g_app->onPress_();
}

void App::onPress_() {
  const uint32_t nowMs = millis();
  if (nowMs - lastPressMs_ < COOLDOWN_MS) {
    Serial.println(F("[BTN] press ignored (cooldown)"));
    return;
  }
  lastPressMs_ = nowMs;

  const bool within = g_sched.within();

  if (within) {
    // Guarantee ordering at boundary: announce+summary before first press
    if (!transitionHandled_) {
      if (g_net.connected()) {
        g_notifier.sendOnline();
        g_log.reportAndClear(g_notifier); // builds & sends summary; clears on success
      }
      transitionHandled_ = true;
    }

    if (g_net.connected()) {
      g_notifier.sendPressedWithButton();
    } else {
      Serial.println(F("[NET] not connected; press ignored in-hours"));
    }
  } else {
    const uint32_t epochNow = g_time.epoch(); // 0 if not synced yet (ok)
    g_log.logPress(epochNow);
    Serial.println(F("[SCHED] queued press (off-hours)"));
  }
}

void App::onReleaseThunk_(const String& userName) {
  if (g_app) g_app->onRelease_(userName);
}

void App::onRelease_(const String& userName) {
  time_t now = g_time.epoch();
  const bool within = g_sched.within();

  if (within) {
    Serial.printf("[APP] Door release triggered by: %s\n", userName.c_str());
    g_relay.trigger();
    if (now > 0 && g_net.connected()) {
      g_notifier.sendReleaseConfirm(userName, now);
    } else {
      Serial.println(F("[APP] Unable to send release confirmation"));
    }
  } else {
    Serial.printf("[APP] Door release rejected (off-hours) by: %s\n", userName.c_str());
    if (now > 0 && g_net.connected()) {
      g_notifier.sendReleaseRejected(userName, now);
    } else {
      Serial.println(F("[APP] Unable to send rejection notice"));
    }
  }
}

void App::handleEdge_(int edge) {
  if (edge == 1) { // Entered working hours
    Serial.println(F("[SCHED] Entered working hours"));
    if (g_net.connected()) {
      g_notifier.sendOnline();
      g_log.reportAndClear(g_notifier);
    }
    transitionHandled_ = true;
  }
  else if (edge == 2) { // Left working hours
    Serial.println(F("[SCHED] Left working hours"));
    if (g_net.connected()) {
      g_notifier.sendOffline();
    }
  }
}
