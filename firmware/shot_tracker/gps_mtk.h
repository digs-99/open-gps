#pragma once
#include <Arduino.h>
#include <Adafruit_GPS.h>
#include "config.h"

// Adafruit Ultimate GPS FeatherWing (MTK3339 / PA1616), NMEA over the Feather's UART.
// Included by gps.h; provides the same GpsReceiver interface as the u-blox driver there.
//
// NMEA has no horizontal accuracy field, so hacc_mm is estimated as HDOP x GPS_HDOP_UERE_MM.
// The module outputs GGA, GSA then RMC each epoch; a fix is reported once the epoch's RMC
// arrives, so it carries that epoch's position, fix type, satellites and HDOP together.
class GpsReceiver {
 public:
  bool begin(HardwareSerial& port) {
    port_ = &port;
    port.setRxBufferSize(1024);  // covers a slow flash save or screen push at GPS_BAUD
    // A fresh module talks 9600. One with a backup coin cell may still be at GPS_BAUD from the
    // last boot and just ignores this.
    port.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    sendPmtk("PMTK251,%lu", (unsigned long)GPS_BAUD);
    port.flush();
    delay(50);
    port.end();
    port.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    sendPmtk("PMTK314,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0");  // RMC, GGA, GSA only
    sendPmtk("PMTK220,%u", 1000 / GPS_NAV_HZ);
    // UART has no ACK, so wait for the first valid sentence (it also wakes a module left in standby).
    GpsFix ignored;
    for (uint32_t t0 = millis(); !present_ && millis() - t0 < 1500; delay(5)) read(ignored);
    return present_;
  }

  // Becomes true on the first valid sentence, so a wing that starts late is still picked up.
  bool present() const { return present_; }

  bool read(GpsFix& f) {
    if (!port_) return false;
    while (port_->available()) {
      char c = port_->read();
      if (c == '$') len_ = 0;
      if (c == '\r' || c == '\n') {
        if (!len_) continue;
        line_[len_] = 0;
        len_ = 0;
        if (!nmea_.parse(line_)) continue;
        present_ = true;
        if (strcmp(nmea_.lastSentence, "RMC")) continue;
        fill(f);
        return true;
      }
      if (len_ < sizeof line_ - 1) line_[len_++] = c;
    }
    return false;
  }

  void standby() { if (present_) sendPmtk("PMTK161,0"); }  // the 3V rail stays up; any byte wakes it

 private:
  void fill(GpsFix& f) {
    f.ok = nmea_.fix && nmea_.fixquality > 0;
    f.fixType = f.ok ? (nmea_.fixquality_3d == 3 ? 3 : 2) : 0;
    f.siv = nmea_.satellites;
    f.lat_e7 = nmea_.latitude_fixed;
    f.lon_e7 = nmea_.longitude_fixed;
    f.hacc_mm = nmea_.HDOP > 0 ? (uint32_t)(nmea_.HDOP * GPS_HDOP_UERE_MM) : UINT32_MAX;
    f.unix = nmea_.year ? unixTime(2000 + nmea_.year, nmea_.month, nmea_.day,
                                   nmea_.hour, nmea_.minute, nmea_.seconds) : 0;
  }

  // Days-from-civil (Howard Hinnant); avoids depending on the TZ setting that mktime() uses.
  static uint32_t unixTime(int y, unsigned m, unsigned d, unsigned hh, unsigned mm, unsigned ss) {
    y -= m <= 2;
    int era = y / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    uint32_t days = (uint32_t)(era * 146097 + (int)doe - 719468);
    return days * 86400UL + hh * 3600UL + mm * 60UL + ss;
  }

  // Sends "$<body>*<checksum>" with the NMEA XOR checksum filled in.
  void sendPmtk(const char* fmt, ...) {
    char body[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(body, sizeof body, fmt, args);
    va_end(args);
    uint8_t sum = 0;
    for (const char* p = body; *p; p++) sum ^= (uint8_t)*p;
    port_->printf("$%s*%02X\r\n", body, sum);
  }

  HardwareSerial* port_ = nullptr;
  Adafruit_GPS nmea_;  // parser only; we own the UART
  char line_[120];     // NMEA sentences are at most 82 chars
  size_t len_ = 0;
  bool present_ = false;
};
