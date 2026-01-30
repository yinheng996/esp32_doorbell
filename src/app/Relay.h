#pragma once
#include <Arduino.h>

class Relay {
public:
  Relay(uint8_t pin, bool activeLow, uint32_t pulseMs);
  void begin();
  void trigger();              // start pulse
  void loop();                 // manage timing

private:
  uint8_t pin_;
  bool activeLow_;
  uint32_t pulseMs_;
  bool active_ {false};
  uint32_t startMs_ {0};
};
