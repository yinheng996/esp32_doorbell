#include "Button.h"

// Static member for ISR access
Button* Button::instance_ = nullptr;

Button::Button(uint8_t pin, bool activeLow, uint32_t debounceMs)
: pin_(pin), activeLow_(activeLow), debounceMs_(debounceMs) {}

void Button::begin(Callback cb) {
  cb_ = cb;
  instance_ = this;
  
  pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT);
  
  // Attach hardware interrupt - use FALLING for active-low, RISING for active-high
  // This ensures interrupt only fires on press edge, not release edge
  int mode = activeLow_ ? FALLING : RISING;
  attachInterrupt(digitalPinToInterrupt(pin_), isrHandler_, mode);
  
  Serial.printf("[BTN] Interrupt attached to pin %d (mode: %s)\n", pin_, activeLow_ ? "FALLING" : "RISING");
}

// ISR - MUST be fast, no Serial, no complex logic
void IRAM_ATTR Button::isrHandler_() {
  if (instance_) {
    instance_->handleInterrupt_();
  }
}

// Called by ISR - latch a press-edge event
void IRAM_ATTR Button::handleInterrupt_() {
  triggered_ = true;
  triggerTimeMs_ = millis();
}

// Main loop checks flag and applies debouncing
void Button::loop() {
  if (!triggered_) return;
  
  unsigned long now = millis();
  unsigned long timeSinceTrigger = now - triggerTimeMs_;
  
  // Debounce: wait for signal to stabilize
  if (timeSinceTrigger < debounceMs_) return;
  
  // Clear the trigger flag
  triggered_ = false;
  
  // Interrupt mode is configured for the press edge only (FALLING/RISING).
  // Once debounced, treat the latched edge as a valid press event.
  if ((now - lastFireMs_) > debounceMs_) {
    lastFireMs_ = now;
    if (cb_) cb_();
  }
}
