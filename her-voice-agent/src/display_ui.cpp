// ============================================================================
//  display_ui.cpp  -  STUB. Replace with the AMOLED + LVGL setup from the
//  Waveshare demo. A nice first vibe-coding task once audio works:
//  "draw a soft glowing orb that pulses faster when LISTENING, swirls when
//   THINKING, and ripples in time with SPEAKING."  -- the perfect "Her" face.
// ============================================================================
#include "display_ui.h"

namespace ui {

bool begin() {
  // TODO: init QSPI AMOLED panel + touch + LVGL display/indev drivers
  //       (copy driver init + pins from Waveshare's LVGL demo).
  Serial.println("[ui] STUB begin() -- fill from Waveshare LVGL demo");
  return true;
}

void setState(State s) {
  const char *names[] = {"idle", "listening", "thinking", "speaking", "error"};
  Serial.printf("[ui] state -> %s\n", names[s]);
  // TODO: animate the orb for state `s`.
}

void showText(const String &t) {
  Serial.printf("[ui] caption: %s\n", t.c_str());
  // TODO: render `t` on screen.
}

void tick() {
  // TODO: lv_timer_handler();
}

} // namespace ui
