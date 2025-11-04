#include "BusinessHours.h"

BusinessHours::BusinessHours(const DayWindows (&hours)[7],
                             const char* tz,
                             bool allowBeforeSync,
                             uint32_t validEpoch)
: hours_(hours),
  tz_(tz),
  allowBeforeSync_(allowBeforeSync),
  validEpoch_(validEpoch) {}

bool BusinessHours::within(const tm& lt) const {
  const int dow = lt.tm_wday; // 0=Sun..6=Sat
  if (dow < 0 || dow > 6) return false;

  const DayWindows& dw = hours_[dow];
  if (dw.count == 0) return false;

  const int mins = minutesSinceMidnight_(lt);
  for (uint8_t i = 0; i < dw.count; ++i) {
    const auto& w = dw.windows[i];
    if (w.start_min < w.end_min) {
      if (mins >= (int)w.start_min && mins < (int)w.end_min) return true;
    }
  }
  return false;
}

bool BusinessHours::withinNow() const {
  time_t now = time(nullptr);
  if ((uint32_t)now < validEpoch_) return allowBeforeSync_; // before sync policy

  struct tm lt{};
  localtime_r(&now, &lt);
  return within(lt);
}
