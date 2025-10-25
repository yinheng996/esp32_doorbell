#include "Scheduler.h"

Scheduler::Scheduler(FnWithin fnWithin) : fn_(fnWithin) {}

void Scheduler::begin(uint32_t pollMs) {
  pollMs_ = pollMs;
  last_   = fn_ ? fn_() : false;
  lastTick_ = millis();
}

Scheduler::Edge Scheduler::poll() {
  uint32_t now = millis();
  if (now - lastTick_ < pollMs_) return Edge::None;
  lastTick_ = now;

  bool cur = fn_ ? fn_() : false;
  if (cur != last_) {
    Edge e = cur ? Edge::Entered : Edge::Left;
    last_ = cur;
    return e;
  }
  return Edge::None;
}
