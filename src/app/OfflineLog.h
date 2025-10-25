#pragma once
#include <Arduino.h>

class Notifier;

class OfflineLog {
public:
  OfflineLog(const char* path, size_t rotateKB);
  void begin(bool formatOnFail=true);
  void logPress(uint32_t epochSecs);        // 0 allowed
  bool reportAndClear(Notifier& notifier);  // summary via Notifier; clear on success
  bool clear();

private:
  String buildSummary_();                   // HH:MM lines (SGT)
  const char* path_;
  size_t rotateKB_;
  bool fsReady_ = false;
};
