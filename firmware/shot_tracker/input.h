#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

struct InputEvents {
  int16_t turn = 0;      // detents since last poll (+ clockwise)
  bool click = false;    // short press, fired on release
  bool longPress = false;// fired once when held past LONG_PRESS_MS
  bool any() const { return turn || click || longPress; }
};

#if BOARD_FEATHER_REVTFT
#include "buttons.h"  // the Feather's three buttons and NeoPixel stand in for the Twist
#else
#include <SparkFun_Qwiic_Twist_Arduino_Library.h>

// Polls the Qwiic Twist. Click vs long-press is timed here rather than using the
// Twist's latched click flag, so a long press never also produces a click.
class Input {
 public:
  bool begin(TwoWire& wire) {
    ok_ = twist_.begin(wire);
    if (ok_) { twist_.getDiff(true); twist_.clearInterrupts(); }
    return ok_;
  }
  bool present() const { return ok_; }

  InputEvents poll() {
    InputEvents e;
    if (!ok_) return e;
    uint32_t now = millis();
    if (now - lastPoll_ < 15) return e;  // ~60 Hz is plenty and keeps the I2C bus free
    lastPoll_ = now;

    e.turn = twist_.getDiff(true);
    bool down = twist_.isPressed();
    if (down && !wasDown_) { downAt_ = now; longFired_ = false; }
    if (down && !longFired_ && now - downAt_ >= LONG_PRESS_MS) { e.longPress = true; longFired_ = true; }
    if (!down && wasDown_ && !longFired_ && now - downAt_ > 25) e.click = true;
    wasDown_ = down;
    if (e.any()) lastActivity_ = now;
    return e;
  }

  uint32_t lastActivity() const { return lastActivity_; }

  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t c = ((uint32_t)r << 16) | (g << 8) | b;
    if (!ok_ || c == color_) return;  // avoid needless I2C writes
    color_ = c;
    twist_.setColor(r, g, b);
  }

 private:
  TWIST twist_;
  bool ok_ = false;
  uint32_t lastPoll_ = 0, downAt_ = 0, lastActivity_ = 0;
  bool wasDown_ = false, longFired_ = false;
  uint32_t color_ = 0xFFFFFFFF;
};
#endif
