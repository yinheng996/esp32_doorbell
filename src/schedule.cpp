#include <Arduino.h>
#include <time.h>
#include "schedule.h"

// Multiple NTP servers; any working one will sync
static const char* NTP1 = "time.google.com";
static const char* NTP2 = "time.cloudflare.com";
static const char* NTP3 = "pool.ntp.org";

static inline int minutesSinceMidnight(const tm& lt) {
  return lt.tm_hour * 60 + lt.tm_min;
}

void scheduleSetupTime() {
  // Set TZ + start SNTP asynchronously
  configTzTime(SCHED_TZ, NTP1, NTP2, NTP3);
}

bool scheduleTimeSynced() {
  time_t now = time(nullptr);
  return now >= SCHED_VALID_EPOCH;
}

bool scheduleEnsureTime(uint32_t wait_ms) {
  scheduleSetupTime();
  uint32_t start = millis();
  while ((millis() - start) < wait_ms) {
    if (scheduleTimeSynced()) return true;
    delay(250);
  }
  return scheduleTimeSynced();
}

bool schedulePrintCurrentLocalTime() {
  time_t now = time(nullptr);
  if (now <= 0) {
    Serial.println("[TIME] not set");
    return false;
  }
  struct tm lt;
  localtime_r(&now, &lt);
  char buf[64];
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &lt);
  Serial.print("[TIME] "); Serial.println(buf);
  return true;
}

bool isWithinSchedule() {
  if (!scheduleTimeSynced()) {
    return SCHED_ALLOW_BEFORE_SYNC;
  }

  time_t now = time(nullptr);
  struct tm lt;
  localtime_r(&now, &lt);                  // 0=Sun ... 6=Sat
  const int mins = minutesSinceMidnight(lt);

  if (lt.tm_wday >= 1 && lt.tm_wday <= 4) {        // Mon–Thu
    return mins >= MON_THU_START && mins < MON_THU_END;
  } else if (lt.tm_wday == 5) {                    // Fri
    return mins >= FRI_START && mins < FRI_END;
  }
  return false;                                    // Sat/Sun
}
