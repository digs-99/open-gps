#pragma once
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include "config.h"

// Peripheral power rails and deep sleep. begin() turns the rails on at boot (they may still be
// held off from the last power-off); sleep() cuts them and never returns. Only RESET (or the EN
// switch) wakes the board.
namespace power {

#if BOARD_FEATHER_REVTFT

// TFT_I2C_POWER feeds both the TFT and the STEMMA QT port. The GPS FeatherWing runs off the 3V
// rail, which stays up, so the app puts it in standby before calling sleep().
static const int RAILS[] = {TFT_I2C_POWER, NEOPIXEL_POWER, TFT_BACKLITE};

inline void begin() {
  gpio_deep_sleep_hold_dis();
  for (int pin : RAILS) {
    gpio_hold_dis((gpio_num_t)pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }
  delay(150);
}

[[noreturn]] inline void sleep() {
  for (int pin : RAILS) {
    digitalWrite(pin, LOW);
    gpio_hold_en((gpio_num_t)pin);
  }
  gpio_deep_sleep_hold_en();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_deep_sleep_start();
  while (true) {}
}

#else

inline void begin() {
  gpio_hold_dis((gpio_num_t)PIN_QWIIC_EN);
  gpio_deep_sleep_hold_dis();
  pinMode(PIN_QWIIC_EN, OUTPUT);
  digitalWrite(PIN_QWIIC_EN, HIGH);
  delay(150);
}

[[noreturn]] inline void sleep() {
  // Cut the Qwiic rail (GPS, OLED, Twist) and hold it low through deep sleep.
  // Tip: cut jumper JP2 on the Thing Plus so the rail defaults OFF and this
  // pin isn't fighting its 10k pull-up (~330 uA) all night.
  digitalWrite(PIN_QWIIC_EN, LOW);
  gpio_hold_en((gpio_num_t)PIN_QWIIC_EN);
  gpio_deep_sleep_hold_en();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);  // only RESET (or the EN switch) wakes us
  esp_deep_sleep_start();
  while (true) {}
}

#endif

}  // namespace power
