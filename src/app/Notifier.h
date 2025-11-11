#pragma once
#include <Arduino.h>

// Forward declarations
class DoorController;

class Notifier {
public:
  Notifier(const char* botToken, const char* chatId, const char* doorName);
  void begin();
  void loop();
  bool sendOnline();
  bool sendOffline();
  bool sendPressed();
  bool sendSummary(const String& text);
  void setDoorController(DoorController* doorCtrl);

private:
  void handleNewMessages(int numNewMessages);
  void handleCallbackQuery(String queryId, String queryData, String chatId);
  void answerCallbackQuery(String queryId);
  bool ensureDnsReady_();
  bool sendTelegram_(const String& text, bool html=false);
  bool sendTelegramWithKeyboard_(const String& text, const String& keyboardJson);
  bool sendTelegramToChat_(const String& chatId, const String& text, bool html=false);
  const char* bot_;
  const char* chat_;
  const char* door_;
  DoorController* doorController_;
};
