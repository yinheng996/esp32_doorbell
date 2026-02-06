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

// Called by ISR - record the event and pin state
void IRAM_ATTR Button::handleInterrupt_() {
  triggerState_ = digitalRead(pin_);
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
  
  // Use the pin state captured at interrupt time (not current state)
  bool pressed = activeLow_ ? (triggerState_ == LOW) : (triggerState_ == HIGH);
  
  // Clear the trigger flag
  triggered_ = false;
  
  // Fire callback only on press edge, with cooldown
  if (pressed && (now - lastFireMs_) > debounceMs_) {
    lastFireMs_ = now;
    if (cb_) cb_();
  }
}
