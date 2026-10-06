#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_ST7789.h>
#include <SparkFun_Qwiic_OLED.h>  // COLOR_WHITE/BLACK and the QwFont fonts, shared with the OLED build
#include "config.h"

// The subset of the Qwiic OLED API that Ui uses, drawn on the Reverse TFT Feather's 240x135
// ST7789. The UI still draws a 128x64 monochrome frame with the OLED's fonts, so every screen is
// laid out exactly as on the OLED. display() scales that frame up (15/8 across, 2x down, centred
// vertically) and only sends the rows that changed.
class TftDisplay {
 public:
  static constexpr int W = 128, H = 64;

  bool begin(TwoWire&) {
    tft_.init(135, 240);
    tft_.setRotation(TFT_ROTATION);
    tft_.fillScreen(TFT_BG);
    memset(shown_, 0xFF, sizeof shown_);  // forces a full push on the first display()
    return true;
  }
  void displayPower(bool on) {
    tft_.enableSleep(!on);
    digitalWrite(TFT_BACKLITE, on ? HIGH : LOW);
  }

  void erase() { memset(buf_, 0, sizeof buf_); }

  void display() {
    static constexpr int PW = 240, Y0 = (135 - 2 * H) / 2;
    uint16_t line[PW];
    tft_.startWrite();
    for (int y = 0; y < H; y++) {
      const uint8_t* row = buf_ + y * ROW_BYTES;
      if (!memcmp(row, shown_ + y * ROW_BYTES, ROW_BYTES)) continue;
      for (int px = 0; px < PW; px++) {
        int x = px * W / PW;
        line[px] = (row[x >> 3] & (0x80 >> (x & 7))) ? TFT_FG : TFT_BG;
      }
      tft_.setAddrWindow(0, Y0 + 2 * y, PW, 2);
      tft_.writePixels(line, PW);
      tft_.writePixels(line, PW);
    }
    tft_.endWrite();
    memcpy(shown_, buf_, sizeof buf_);
  }

  void setFont(QwFont& font) { font_ = &font; }

  // Same metrics and glyph layout as SparkFun's renderer, including the 5x7 font's extra column.
  unsigned getStringWidth(const char* s) const {
    return font_ ? strlen(s) * (font_->width + margin()) : 0;
  }
  void text(int x, int y, const char* s, uint8_t clr) {
    if (!font_ || !s) return;
    const uint8_t* data = font_->data();
    int rows = max(1, font_->height / 8);
    int perRow = font_->map_width / font_->width;
    for (; *s; s++, x += font_->width + margin()) {
      unsigned c = (uint8_t)*s - font_->start;
      if (c >= font_->n_chars) continue;
      int index = c / perRow * font_->map_width * rows + c % perRow * font_->width;
      for (int r = 0; r < rows; r++)
        for (int i = 0; i < font_->width; i++) {
          uint8_t bits = pgm_read_byte(data + index + i + r * font_->map_width);
          for (int j = 0; j < 8; j++)
            if (bits & (1 << j)) pixel(x + i, y + r * 8 + j, clr);
        }
    }
  }

  void pixel(int x, int y, uint8_t clr = COLOR_WHITE) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    uint8_t& b = buf_[y * ROW_BYTES + (x >> 3)];
    uint8_t bit = 0x80 >> (x & 7);
    if (clr == COLOR_WHITE) b |= bit; else b &= ~bit;
  }
  void line(int x0, int y0, int x1, int y1, uint8_t clr = COLOR_WHITE) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    for (int err = dx + dy;;) {
      pixel(x0, y0, clr);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }
  void rectangle(int x, int y, int w, int h, uint8_t clr = COLOR_WHITE) {
    if (w <= 0 || h <= 0) return;
    line(x, y, x + w - 1, y, clr);
    line(x, y + h - 1, x + w - 1, y + h - 1, clr);
    line(x, y, x, y + h - 1, clr);
    line(x + w - 1, y, x + w - 1, y + h - 1, clr);
  }
  void rectangleFill(int x, int y, int w, int h, uint8_t clr = COLOR_WHITE) {
    for (int j = y; j < y + h; j++)
      for (int i = x; i < x + w; i++) pixel(i, j, clr);
  }

 private:
  static constexpr int ROW_BYTES = W / 8;
  int margin() const { return font_->height <= 8 ? 1 : 0; }  // SparkFun spaces the 5x7 font by one

  Adafruit_ST7789 tft_{TFT_CS, TFT_DC, TFT_RST};
  QwFont* font_ = nullptr;
  uint8_t buf_[W * H / 8];    // 1 bit per pixel, rows top to bottom, MSB = leftmost
  uint8_t shown_[W * H / 8];  // what the panel shows now
};
