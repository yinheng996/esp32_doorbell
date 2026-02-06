#include "Scheduler.h"

Scheduler::Scheduler(FnWithin fnWithin) : fn_(fnWithin) {}

void Scheduler::begin(uint32_t pollMs) {
  pollMs_ = pollMs;
  last_   = fn_ ? fn_() : false;
  lastTick_ = millis();
  time_t t = time(nullptr);
  lastMinute_ = (t > 0) ? (uint32_t)(t / 60) : 0xFFFFFFFF;
}

Scheduler::Edge Scheduler::poll() {
  const uint32_t nowMs = millis();
  time_t t = time(nullptr);
  const uint32_t minute = (t > 0) ? (uint32_t)(t / 60) : 0;
  const bool minuteChanged = (lastMinute_ != minute);
  if (minuteChanged) lastMinute_ = minute;

  const bool pollDue = (nowMs - lastTick_ >= pollMs_);
  if (!pollDue && !minuteChanged) return Edge::None;
  if (pollDue) lastTick_ = nowMs;

  bool cur = fn_ ? fn_() : false;
  if (cur != last_) {
    Edge e = cur ? Edge::Entered : Edge::Left;
    last_ = cur;
    return e;
  }
  return Edge::None;
}
