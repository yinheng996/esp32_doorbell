#include "Button.h"

Button::Button(uint8_t pin, bool activeLow, uint32_t debounceMs)
: pin_(pin), activeLow_(activeLow), debounceMs_(debounceMs) {}

void Button::begin(Callback cb) {
  cb_ = cb;
  pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT);
  lastStable_ = digitalRead(pin_);
  lastRead_   = lastStable_;
  lastChangeMs_ = millis();
}

void Button::loop() {
  int raw = digitalRead(pin_);
  unsigned long now = millis();
  if (raw != lastRead_) { lastRead_ = raw; lastChangeMs_ = now; }
  if ((now - lastChangeMs_) > debounceMs_) {
    if (lastStable_ != lastRead_) {
      lastStable_ = lastRead_;
      // fire on "pressed" edge
      bool pressed = activeLow_ ? (lastStable_ == LOW) : (lastStable_ == HIGH);
      if (pressed && cb_) cb_();
    }
  }
}
