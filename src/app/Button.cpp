#include "Button.h"

Button* Button::instance_ = nullptr;

Button::Button(uint8_t pin, bool activeLow, uint32_t debounceMs)
: pin_(pin), activeLow_(activeLow), debounceMs_(debounceMs) {
  instance_ = this;
}

void IRAM_ATTR Button::isr_() {
  if (instance_) {
    instance_->interruptFlag_ = true;
  }
}

void Button::begin(Callback cb) {
  cb_ = cb;
  pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT);
  lastStable_ = digitalRead(pin_);
  lastRead_   = lastStable_;
  lastChangeMs_ = millis();
  
  // Attach interrupt for immediate detection
  attachInterrupt(digitalPinToInterrupt(pin_), isr_, CHANGE);
}

void Button::loop() {
  // Check if interrupt occurred
  if (interruptFlag_) {
    interruptFlag_ = false;
    checkState_();
  }
  
  // Also do polling as backup (for debouncing)
  unsigned long now = millis();
  if ((now - lastChangeMs_) > debounceMs_) {
    int raw = digitalRead(pin_);
    if (raw != lastRead_) {
      lastRead_ = raw;
      lastChangeMs_ = now;
      checkState_();
    }
  }
}

void Button::checkState_() {
  unsigned long now = millis();
  int raw = digitalRead(pin_);
  
  if (raw != lastRead_) {
    lastRead_ = raw;
    lastChangeMs_ = now;
  }
  
  if ((now - lastChangeMs_) > debounceMs_) {
    if (lastStable_ != lastRead_) {
      lastStable_ = lastRead_;
      // fire on "pressed" edge
      bool pressed = activeLow_ ? (lastStable_ == LOW) : (lastStable_ == HIGH);
      if (pressed && cb_) cb_();
    }
  }
}
