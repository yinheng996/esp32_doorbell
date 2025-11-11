#pragma once
#include <Arduino.h>

class DoorController {
public:
  using FnWithinHours = bool(*)();
  
  DoorController(int doorPin, const char* doorName, FnWithinHours fnWithinHours);
  void begin();
  void loop();
  void openDoor();
  void closeDoor();
  int getDoorStatus() const;
  bool isWithinWorkingHours() const;
  const char* getDoorName() const { return doorName_; }

private:
  void handleRequest();
  void handleOpenDoor();
  void handleCloseDoor();
  
  int doorPin_;
  int doorStatus_;
  const char* doorName_;
  FnWithinHours fnWithinHours_;
};

