#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Hardware. The board you compile for picks the build:
//   SparkFun Thing Plus ESP32-S3 (default) - Qwiic OLED, Qwiic Twist, NEO-M9N GPS
//   Adafruit ESP32-S3 Reverse TFT Feather  - built-in TFT and buttons, Ultimate GPS FeatherWing
// ---------------------------------------------------------------------------
#if defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S3_REVTFT)
#define BOARD_FEATHER_REVTFT 1
#else
#define BOARD_FEATHER_REVTFT 0
#endif

#if BOARD_FEATHER_REVTFT
// Adafruit ESP32-S3 Reverse TFT Feather (TFT/NeoPixel pins come from the board variant)
constexpr int PIN_SDA = SDA;  // STEMMA QT; also the MAX17048 fuel gauge
constexpr int PIN_SCL = SCL;
constexpr uint32_t I2C_HZ = 400000;
constexpr int PIN_GPS_RX = RX;  // Ultimate GPS FeatherWing TX -> Feather RX
constexpr int PIN_GPS_TX = TX;
constexpr uint32_t GPS_BAUD = 57600;  // switched up from the module's 9600 default at boot
constexpr float GPS_HDOP_UERE_MM = 3000;  // NMEA has no accuracy field: estimate = HDOP x this (~3 m CEP spec)
// Buttons stand in for the dial: up/down turn it, select clicks it, holding select opens the menu.
// If up and down feel backwards on your board, swap these two.
constexpr int PIN_BTN_UP = 0;      // D0, active LOW (also BOOT)
constexpr int PIN_BTN_SELECT = 1;  // D1, active HIGH
constexpr int PIN_BTN_DOWN = 2;    // D2, active HIGH
constexpr uint32_t BUTTON_REPEAT_DELAY_MS = 400;  // hold up/down this long to auto-repeat
constexpr uint32_t BUTTON_REPEAT_MS = 120;
constexpr uint8_t TFT_ROTATION = 3;        // 1 turns the screen upside down
constexpr uint16_t TFT_FG = 0xFFFF;        // RGB565 white
constexpr uint16_t TFT_BG = 0x0000;        // black
#else
// SparkFun Thing Plus ESP32-S3 (from SparkFun's KiCad netlist)
constexpr int PIN_QWIIC_EN = 45;  // drives the RT9080 that powers the Qwiic rail (GPS, OLED, Twist)
constexpr int PIN_SDA = 8;
constexpr int PIN_SCL = 9;
constexpr uint32_t I2C_HZ = 400000;
#endif

// ---------------------------------------------------------------------------
// Round
// ---------------------------------------------------------------------------
constexpr uint8_t NUM_HOLES = 18;
constexpr uint16_t MAX_SHOTS = 250;  // per round; ~36 bytes each

// Club list shown on the dial. Order = dial order. Keep names <= 14 chars.
// The putter must be named exactly PUTTER_NAME (putts record one point, no landing).
#define PUTTER_NAME "Putter"
static const char* const CLUBS[] = {
    "Driver", "3 Wood", "5 Wood", "Hybrid", "4 Iron", "5 Iron", "6 Iron",
    "7 Iron", "8 Iron", "9 Iron", "PW",     "GW",     "SW",     "LW",
    PUTTER_NAME,
};
constexpr uint8_t NUM_CLUBS = sizeof(CLUBS) / sizeof(CLUBS[0]);
// The dial has one extra slot after the clubs: "Hole out" (finishes the hole).
constexpr uint8_t DIAL_HOLE_OUT = NUM_CLUBS;

// ---------------------------------------------------------------------------
// GPS
// ---------------------------------------------------------------------------
constexpr uint8_t GPS_NAV_HZ = 4;                 // solutions per second
constexpr uint32_t GPS_MARK_WINDOW_MS = 2500;     // average fixes for this long when marking a point
constexpr uint8_t GPS_MARK_MIN_SAMPLES = 3;       // below this the mark counts as failed
constexpr uint32_t GPS_MAX_GOOD_HACC_MM = 8000;   // fixes worse than this are ignored while marking

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------
constexpr bool USE_YARDS = true;             // false = metres
constexpr uint32_t LONG_PRESS_MS = 700;      // hold the dial this long to open the menu
constexpr uint32_t RESULT_SHOW_MS = 4000;    // how long the "152 yds" result stays up
constexpr uint32_t UI_REFRESH_MS = 250;
