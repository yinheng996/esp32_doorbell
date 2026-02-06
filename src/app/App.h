#pragma once
#include <Arduino.h>

class App {
public:
  void begin();
  void loop();

private:
  // Event handlers
  static void onPressThunk_();              // static trampoline -> calls instance
  static void onReleaseThunk_(const String& userName); // static trampoline for door release
  void        onPress_();
  void        onRelease_(const String& userName);
  void        handleEdge_(int edge);        // 0=None, 1=Entered, 2=Left

  // Ordering guard: ensure "online + summary" before first in-hours press
  bool transitionHandled_ = true;
  uint32_t lastPressMs_ = 0;
};
