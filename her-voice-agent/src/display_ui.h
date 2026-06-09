// ============================================================================
//  display_ui.h  -  the round AMOLED face. Another HARDWARE HOOK (stubbed).
//  Fill from Waveshare's LVGL demo for this board (correct QSPI display +
//  touch pins). Until then, states just print to Serial so the rest works.
// ============================================================================
#pragma once
#include <Arduino.h>

namespace ui {

enum State { IDLE, LISTENING, THINKING, SPEAKING, ERROR };

bool begin();                       // init AMOLED + LVGL
void setState(State s);             // drive the on-screen orb / mood
void showText(const String &t);     // caption the transcript/reply
void tick();                        // call often; pumps LVGL timers

} // namespace ui
