#pragma once
#include <Arduino.h>

class DoorController {
public:
  using FnWithinHours = bool(*)();
  
  DoorController(const char* botToken, int doorPin, const char* doorName, FnWithinHours fnWithinHours);
  void begin();
  void loop();
  void openDoor();
  void closeDoor();
  int getDoorStatus() const;

private:
  void handleNewMessages(int numNewMessages);
  void handleCallbackQuery(String queryId, String queryData);
  void handleRequest();
  void handleOpenDoor();
  void handleCloseDoor();
  bool isWithinWorkingHours() const;
  
  const char* botToken_;
  int doorPin_;
  int doorStatus_;
  const char* doorName_;
  FnWithinHours fnWithinHours_;
};

