#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// Included by input.h on the Reverse TFT Feather. The three buttons stand in for the Twist:
// up/down turn the dial one detent (repeating while held), a short press of select clicks, and
// holding select is the long press. The onboard NeoPixel takes the place of the dial's LED.
// Click vs long-press is timed the same way as the Twist, so a long press never also clicks.
class Input {
 public:
  bool begin(TwoWire&) {
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_SELECT, INPUT_PULLDOWN);
    pinMode(PIN_BTN_DOWN, INPUT_PULLDOWN);
    return true;
  }
  bool present() const { return true; }

  InputEvents poll() {
    InputEvents e;
    uint32_t now = millis();
    if (now - lastPoll_ < 15) return e;  // same rate as the Twist; also debounces
    lastPoll_ = now;

    e.turn = up_.step(digitalRead(PIN_BTN_UP) == LOW, now) ? -1 : 0;
    if (down_.step(digitalRead(PIN_BTN_DOWN) == HIGH, now)) e.turn += 1;

    bool down = digitalRead(PIN_BTN_SELECT) == HIGH;
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
    if (c == color_) return;
    color_ = c;
    rgbLedWrite(PIN_NEOPIXEL, r, g, b);
  }

 private:
  // One detent on press, then auto-repeat while held.
  struct Repeat {
    bool wasDown = false;
    uint32_t nextAt = 0;
    bool step(bool down, uint32_t now) {
      bool fire = false;
      if (down && !wasDown) { fire = true; nextAt = now + BUTTON_REPEAT_DELAY_MS; }
      else if (down && (int32_t)(now - nextAt) >= 0) { fire = true; nextAt = now + BUTTON_REPEAT_MS; }
      wasDown = down;
      return fire;
    }
  };

  Repeat up_, down_;
  uint32_t lastPoll_ = 0, downAt_ = 0, lastActivity_ = 0;
  bool wasDown_ = false, longFired_ = false;
  uint32_t color_ = 0xFFFFFFFF;
};
