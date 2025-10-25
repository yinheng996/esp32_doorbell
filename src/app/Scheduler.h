#pragma once
#include <Arduino.h>

class Scheduler {
public:
  enum class Edge { None, Entered, Left };
  using FnWithin = bool(*)();

  explicit Scheduler(FnWithin fnWithin);
  void begin(uint32_t pollMs=1000);
  Edge poll();                // call each loop; detects transitions
  bool within() const { return last_; }

private:
  FnWithin fn_;
  bool last_ {false};
  uint32_t pollMs_ {1000};
  uint32_t lastTick_ {0};
};
