#include "Relay.h"

Relay::Relay(uint8_t pin, bool activeLow, uint32_t pulseMs)
: pin_(pin), activeLow_(activeLow), pulseMs_(pulseMs) {}

void Relay::begin() {
  // Set safe state before pinMode
  digitalWrite(pin_, activeLow_ ? HIGH : LOW);
  pinMode(pin_, OUTPUT);
  Serial.printf("[RELAY] Initialized on pin %d (active %s)\n", 
                pin_, activeLow_ ? "LOW" : "HIGH");
}

void Relay::trigger() {
  if (active_) {
    Serial.println(F("[RELAY] Already active; ignoring trigger"));
    return;
  }
  digitalWrite(pin_, activeLow_ ? LOW : HIGH);
  active_ = true;
  startMs_ = millis();
  Serial.printf("[RELAY] Triggered for %lu ms\n", pulseMs_);
}

void Relay::loop() {
  if (!active_) return;
  if ((millis() - startMs_) >= pulseMs_) {
    digitalWrite(pin_, activeLow_ ? HIGH : LOW);
    active_ = false;
    Serial.println(F("[RELAY] Deactivated"));
  }
}
