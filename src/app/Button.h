#pragma once
#include <Arduino.h>

class Button {
public:
  using Callback = void(*)();

  Button(uint8_t pin, bool activeLow, uint32_t debounceMs);
  void begin(Callback cb);
  void loop();                // call in loop(); fires cb on stable press

private:
  static void isr_();
  static Button* instance_;
  
  void checkState_();
  
  uint8_t pin_;
  bool activeLow_;
  uint32_t debounceMs_;
  Callback cb_ {nullptr};

  volatile bool interruptFlag_ {false};
  int lastStable_ {HIGH};
  int lastRead_   {HIGH};
  unsigned long lastChangeMs_ {0};
};
