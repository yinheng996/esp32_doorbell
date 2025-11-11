#pragma once
#include <Arduino.h>

class DoorController {
public:
  DoorController(const char* botToken, int doorPin);
  void begin();
  void loop();
  void openDoor();
  void closeDoor();
  int getDoorStatus() const;

private:
  void handleNewMessages(int numNewMessages);
  void handleRequest();
  void handleOpenDoor();
  void handleCloseDoor();
  
  const char* botToken_;
  int doorPin_;
  int doorStatus_;
};

