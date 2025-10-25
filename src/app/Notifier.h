#pragma once
#include <Arduino.h>

class Notifier {
public:
  Notifier(const char* botToken, const char* chatId, const char* doorName);
  bool sendOnline();
  bool sendOffline();
  bool sendPressed();
  bool sendSummary(const String& text);

private:
  bool sendTelegram_(const String& text, bool html=false);
  const char* bot_;
  const char* chat_;
  const char* door_;
};
