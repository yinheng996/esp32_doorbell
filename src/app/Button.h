#pragma once
#include <Arduino.h>

class Button {
public:
  using Callback = void(*)();

  Button(uint8_t pin, bool activeLow, uint32_t debounceMs);
  void begin(Callback cb);
  void loop();                // call in loop(); checks ISR flag and fires cb

private:
  static void IRAM_ATTR isrHandler_();  // ISR must be in IRAM on ESP32
  static Button* instance_;              // For ISR access
  
  void handleInterrupt_();
  
  uint8_t pin_;
  bool activeLow_;
  uint32_t debounceMs_;
  Callback cb_ {nullptr};
  
  volatile bool triggered_ {false};           // ISR sets this flag
  volatile unsigned long triggerTimeMs_ {0};  // ISR records timestamp
  volatile int triggerState_ {HIGH};          // ISR records pin state at trigger
  unsigned long lastFireMs_ {0};              // Last time callback fired
};
