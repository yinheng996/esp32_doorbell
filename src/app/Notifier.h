#pragma once
#include <Arduino.h>
#include <time.h>

class Notifier {
public:
  Notifier(const char* botToken, const char* chatId, const char* doorName);
  bool sendOnline();
  bool sendOffline();
  bool sendPressed();
  bool sendPressedWithButton();              // doorbell notification with inline "Release Door" button
  bool sendReleaseConfirm(const String& userName, time_t timestamp); // "Door released\nBy: [user] at HH:MM:SS"
  bool sendReleaseRejected(const String& userName, time_t timestamp); // "Rejected Door Release\n[user] at HH:MM:SS"
  bool sendSummary(const String& text);
  void pollUpdates(void (*onRelease)(const String&)); // poll for callback queries, call onRelease with user info

private:
  bool sendTelegram_(const String& text, bool html=false);
  bool sendTelegramWithButton_(const String& text, const String& buttonText, const String& callbackData);
  const char* bot_;
  const char* chat_;
  const char* door_;
  int32_t lastUpdateId_ {0};
  uint32_t lastPollMs_ {0};
  uint32_t pollIntervalMs_ {500}; // Poll every 500ms
};
