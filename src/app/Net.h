#pragma once
#include <Arduino.h>

class Net {
public:
  Net(const char* ssid, const char* pass, const char* hostname);
  void begin();
  void waitReady(uint32_t ms);
  void loop();                   // optional reconnect/backoff (no-op for now)
  bool connected() const;

private:
  const char* ssid_;
  const char* pass_;
  const char* hostname_;
};
