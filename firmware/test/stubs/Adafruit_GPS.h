#pragma once
#include "Arduino.h"
#include <ctime>
// parse() accepts any "$xxSSS..." line and fills the fields from the sim's position, fix flag and
// clock (the same values the u-blox stub reports), so both builds record identical rounds.
extern double g_lat, g_lon; extern bool g_fix;
class Adafruit_GPS { public:
  char lastSentence[6]={0}; bool fix=false; uint8_t fixquality=0, fixquality_3d=0, satellites=0;
  int32_t latitude_fixed=0, longitude_fixed=0; float HDOP=0;
  uint8_t year=0, month=0, day=0, hour=0, minute=0, seconds=0;
  bool parse(char* s){ if(s[0]!='$'||strlen(s)<6) return false; memcpy(lastSentence,s+3,3); lastSentence[3]=0;
    fix=g_fix; fixquality=g_fix?1:0; fixquality_3d=g_fix?3:1; satellites=g_fix?14:2; HDOP=0.6f;
    latitude_fixed=(int32_t)llround(g_lat*1e7); longitude_fixed=(int32_t)llround(g_lon*1e7);
    time_t t=1790000000+millis()/1000; struct tm tm; gmtime_r(&t,&tm);
    year=tm.tm_year-100; month=tm.tm_mon+1; day=tm.tm_mday; hour=tm.tm_hour; minute=tm.tm_min; seconds=tm.tm_sec; return true; } };
