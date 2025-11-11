#include "Button.h"

Button* Button::instance_ = nullptr;

Button::Button(uint8_t pin, bool activeLow, uint32_t debounceMs)
: pin_(pin), activeLow_(activeLow), debounceMs_(debounceMs) {
  instance_ = this;
}

void IRAM_ATTR Button::isr_() {
  if (instance_) {
    instance_->interruptFlag_ = true;
    instance_->interruptTimeMs_ = millis();
  }
}

void Button::begin(Callback cb) {
  cb_ = cb;
  pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT);
  lastStable_ = digitalRead(pin_);
  lastRead_   = lastStable_;
  lastChangeMs_ = millis();
  
  // Attach interrupt - use FALLING for activeLow (HIGH->LOW), RISING for activeHigh (LOW->HIGH)
  attachInterrupt(digitalPinToInterrupt(pin_), isr_, activeLow_ ? FALLING : RISING);
}

void Button::loop() {
  unsigned long now = millis();
  
  // Check if interrupt occurred and debounce period has elapsed
  if (interruptFlag_ && interruptTimeMs_ > 0) {
    if ((now - interruptTimeMs_) >= debounceMs_) {
      // Debounce period has passed, check the state
      interruptFlag_ = false;
      int raw = digitalRead(pin_);
      lastRead_ = raw;
      lastChangeMs_ = interruptTimeMs_;
      
      // Check if state actually changed and is pressed
      if (lastStable_ != lastRead_) {
        lastStable_ = lastRead_;
        bool pressed = activeLow_ ? (lastStable_ == LOW) : (lastStable_ == HIGH);
        if (pressed && cb_) {
          cb_();
        }
      }
      interruptTimeMs_ = 0;
    }
  }
  
  // Also do polling as backup (for debouncing)
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
  
  // Wait for debounce period, then check if state changed
  if ((now - lastChangeMs_) >= debounceMs_) {
    if (lastStable_ != lastRead_) {
      lastStable_ = lastRead_;
      // fire on "pressed" edge
      bool pressed = activeLow_ ? (lastStable_ == LOW) : (lastStable_ == HIGH);
      if (pressed && cb_) {
        cb_();
      }
    }
  }
}
