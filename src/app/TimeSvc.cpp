#include "TimeSvc.h"
#include <time.h>

static const char* NTP1 = "time.google.com";
static const char* NTP2 = "time.cloudflare.com";
static const char* NTP3 = "pool.ntp.org";

TimeSvc::TimeSvc(const char* tz, uint32_t syncTimeoutMs)
: tz_(tz), timeoutMs_(syncTimeoutMs) {}

void TimeSvc::begin() {
  setenv("TZ", tz_, 1); tzset();
  configTzTime(tz_, NTP1, NTP2, NTP3);
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs_) {
    if (synced()) break;
    delay(250);
  }
}

void TimeSvc::loop() { /* no-op */ }

uint32_t TimeSvc::epoch() const { return (uint32_t)time(nullptr); }

bool TimeSvc::synced() const {
  time_t now = time(nullptr);
  // treat >= 2020-01-01 as "valid" (1577836800)
  return now >= 1577836800;
}

void TimeSvc::printLocal() const {
  time_t now = time(nullptr);
  if (now <= 0) { Serial.println("[TIME] not set"); return; }
  struct tm lt; localtime_r(&now, &lt);
  char buf[64]; strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &lt);
  Serial.print("[TIME] "); Serial.println(buf);
}
