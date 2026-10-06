#pragma once
#include "Arduino.h"
// Records what reaches the 240x135 panel (after rotation) so the sim can check the scaled frame.
extern uint16_t g_panel[135][240]; extern int g_panelRowsPushed, g_panelErrors; extern bool g_panelSleep;
class Adafruit_ST7789 { int x0=0,y0=0,w=0,h=0; long pos=0;
 public: Adafruit_ST7789(int,int,int){} void init(int,int){} void setRotation(int){}
  void fillScreen(uint16_t c){ for(auto& r:g_panel) for(auto& p:r) p=c; }
  void startWrite(){} void endWrite(){}
  void setAddrWindow(uint16_t x,uint16_t y,uint16_t ww,uint16_t hh){ x0=x; y0=y; w=ww; h=hh; pos=0; if(x+ww>240||y+hh>135) g_panelErrors++; g_panelRowsPushed+=hh; }
  void writePixels(uint16_t* c,uint32_t n,bool=true,bool=false){ for(uint32_t i=0;i<n;i++,pos++){ long px=x0+pos%w, py=y0+pos/w;
      if(pos>=(long)w*h||px>=240||py>=135){ g_panelErrors++; continue; } g_panel[py][px]=c[i]; } }
  void enableDisplay(bool){} void enableSleep(bool s){ g_panelSleep=s; } };
