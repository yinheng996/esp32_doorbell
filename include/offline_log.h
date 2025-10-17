#pragma once
#include <Arduino.h>

namespace offlinelog {

// Mount LittleFS (safe to call multiple times)
void begin();

// Record a press captured while notifications are muted/off-hours.
// Pass Unix epoch seconds if available (0 is allowed).
void logPress(uint32_t epochSeconds);

// When working hours start: send a summary via Telegram and clear on success.
// Returns true if sent and cleared (or nothing to send).
bool reportAndClear();

// Optional: immediately clear the log file.
void clearNow();

} // namespace offlinelog