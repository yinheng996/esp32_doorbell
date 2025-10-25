#pragma once
#include <Arduino.h>

class App {
public:
  void begin();
  void loop();

private:
  // Event handlers
  static void onPressThunk_();     // static trampoline -> calls instance
  void        onPress_();
  void        handleEdge_(int edge); // 0=None, 1=Entered, 2=Left

  // Ordering guard: ensure “online + summary” before first in-hours press
  bool transitionHandled_ = true;
};
