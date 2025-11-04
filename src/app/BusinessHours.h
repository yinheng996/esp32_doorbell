#pragma once
#include <Arduino.h>
#include <time.h>
#include "work_hours_types.h" // provides WorkWindow/DayWindows definitions

class BusinessHours {
public:
  // Pass the WORK_HOURS array and policy knobs
  BusinessHours(const DayWindows (&hours)[7],
                const char* tz,
                bool allowBeforeSync,
                uint32_t validEpoch);

  // True if now is within any window for today's day-of-week (local time).
  bool withinNow() const;

  // Helper if you already have a local tm
  bool within(const tm& localTm) const;

  // Expose policy
  const char* tz() const { return tz_; }

private:
  const DayWindows (&hours_)[7];
  const char* tz_;
  bool allowBeforeSync_;
  uint32_t validEpoch_;

  static inline int minutesSinceMidnight_(const tm& lt) {
    return lt.tm_hour * 60 + lt.tm_min;
  }
};
