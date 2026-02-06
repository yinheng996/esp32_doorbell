#pragma once
#include <Arduino.h>

class TimeSvc {
public:
  TimeSvc(const char* tz, uint32_t syncTimeoutMs);
  void begin();                 // configTzTime + optional wait
  void loop();                  // usually no-op
  uint32_t epoch() const;       // 0 if not synced
  bool synced() const;
  void printLocal() const;

private:
  const char* tz_;
  uint32_t timeoutMs_;
  uint32_t lastRetryMs_{0};  // for NTP retry in loop()
};
