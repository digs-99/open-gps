#include "Arduino.h"
#include "Wire.h"
#include "LittleFS.h"
#include "SparkFun_Qwiic_OLED.h"
uint32_t g_millis = 1000; Stream Serial; HardwareSerial Serial1; TwoWire Wire; MemFS g_fs; LittleFSC LittleFS; bool g_slept=false;
int16_t g_twistDiff=0; bool g_twistDown=false; double g_lat=47.600000, g_lon=-122.300000; bool g_fix=true; uint32_t g_lastPvt=0;
int g_pins[64]; uint32_t g_led=0;
uint16_t g_panel[135][240]; int g_panelRowsPushed=0, g_panelErrors=0; bool g_panelSleep=false;
QwiicFont F5{5,8,0,255,1275},F8{8,16,32,96,256},FL{12,48,48,11,132}; std::string g_screen;
#include "../shot_tracker/app.cpp"

static const char* scrName(){ const char* n[]={"Home","Club","Mark","Flight","Result","Menu","Holes","Confirm","Score","GpsInfo"}; return n[(int)scr]; }
void step(uint32_t ms){ for(uint32_t t=0;t<ms;t+=5){ g_millis+=5; loop(); } }
int fails=0;
void expect(bool c,const char* what){ printf("  %s %s\n", c?"ok  ":"FAIL", what); if(!c) fails++; }
#if BOARD_FEATHER_REVTFT
// Buttons: D0 (up) idles HIGH, D1/D2 idle LOW. One press of up/down is one detent.
void press(int pin,uint32_t ms){ g_pins[pin]=!g_pins[pin]; step(ms); g_pins[pin]=!g_pins[pin]; step(40); }
void turn(int n){ for(int i=0;i<abs(n);i++) press(n>0?PIN_BTN_DOWN:PIN_BTN_UP,40); }
void click(){ press(PIN_BTN_SELECT,120); step(20); }
void hold(){ press(PIN_BTN_SELECT,900); step(20); }
// The GPS wing streams GGA, GSA, RMC at GPS_NAV_HZ once the UART is open.
void gpsFeed(){ if(Serial1.baud && millis()-g_lastPvt>=1000/GPS_NAV_HZ){ g_lastPvt=millis(); Serial1.in+="$GPGGA,x*00\r\n$GPGSA,x*00\r\n$GPRMC,x*00\r\n"; } }
// The logical 128x64 frame as read back off the panel, two rows per line in half blocks.
void screen(){ step(300); printf("---- %s ----\n", scrName());
  auto lit=[](int x,int y){ return g_panel[3+2*y][(x*240+127)/128]!=TFT_BG; };
  for(int y=0;y<64;y+=2){ for(int x=0;x<128;x++){ bool t=lit(x,y),b=lit(x,y+1); printf("%s", t&&b?"\u2588":t?"\u2580":b?"\u2584":" "); } printf("\n"); } }
#else
void turn(int n){ g_twistDiff+=n; step(40); }
void click(){ g_twistDown=true; step(120); g_twistDown=false; step(60); }
void hold(){ g_twistDown=true; step(900); g_twistDown=false; step(60); }
void screen(){ step(300); printf("---- %s ----\n%s", scrName(), g_screen.c_str()); }
#endif
void walkYards(double yd){ g_lat += yd*0.9144/111194.9; step(100); }
int menuIndex(const char* label){ for(int i=0;i<menuCount;i++) if(!strcmp(menu[i].label,label)) return i; return -1; }
void pick(const char* label){ int i=menuIndex(label); if(i<0){ printf("  FAIL no menu item %s\n",label); fails++; return;} turn(i-menuSel); click(); }

int main(){
#if BOARD_FEATHER_REVTFT
  g_pins[PIN_BTN_UP]=HIGH; Serial1.feed=gpsFeed;
#endif
  setup(); step(200);
  expect(scr==Scr::Home,"boots to home");
  pick("New round");
  expect(scr==Scr::Club && round_.h.currentHole==1,"new round, hole 1, club select");
  screen();
  // Drive: 250 yds
  click(); expect(scr==Scr::Mark,"marking start");
  step(2600); expect(scr==Scr::Flight,"in flight after mark");
  walkYards(125); screen();
  walkYards(125); click(); step(2600);
  expect(scr==Scr::Result,"result shown"); printf("  drive = %d yds\n", lastResult);
  expect(abs(lastResult-250)<=1,"drive measured ~250");
  screen();
  step(4500); expect(scr==Scr::Club,"back to club select after result");
  // 7 iron 150
  turn(7); expect(dialSel==7,"dial to 7 Iron");
  click(); step(2600); walkYards(150); click(); step(2600);
  expect(abs(lastResult-150)<=1,"7i measured ~150");
  click(); // dismiss result
  // two putts
  turn(putterIndex()-dialSel); expect(isPutter(dialSel),"dial to putter");
  click(); step(2600); expect(scr==Scr::Club && round_.h.shotCount==3,"putt recorded as single point");
  click(); step(2600); expect(round_.h.shotCount==4 && dialSel==putterIndex(),"second putt, dial stays on putter");
  turn(1); expect(dialSel==DIAL_HOLE_OUT,"one click past putter = Hole out");
  screen();
  click(); expect(round_.h.currentHole==2 && round_.strokesOnHole(1)==4,"holed out: H1 = 4 strokes, now H2");
  // Hole 2: start a drive then cancel it from the menu
  turn(-(int)dialSel); click(); step(2600); expect(round_.inFlight(),"H2 drive in flight");
  hold(); expect(scr==Scr::Menu,"long press opens menu");
  screen();
  pick("Cancel this shot"); expect(scr==Scr::Confirm && confirmSel==0,"confirm defaults to No");
  turn(1); click(); expect(round_.h.shotCount==4 && !round_.inFlight() && scr==Scr::Club,"shot cancelled");
  // Reset mid-flight -> resume
  click(); step(2600); walkYards(60);
  expect(scr==Scr::Flight,"in flight before reset");
  round_.reset(); scr=Scr::Home; setup(); step(300);
  expect(scr==Scr::Flight && round_.h.shotCount==5 && round_.h.currentHole==2,"RESET mid-shot resumes in flight");
  walkYards(40); click(); step(2600); expect(abs(lastResult-100)<=1,"landing after resume measures from original start (~100)");
  click();
  // No GPS fix -> prompt -> save without GPS
  g_fix=false; step(3100);
  click(); step(2600); expect(scr==Scr::Menu && !strcmp(menuTitle,"NO GPS FIX"),"no-fix prompt");
  screen();
  pick("Save without GPS"); expect(scr==Scr::Flight && !round_.last()->start.valid,"shot saved without GPS, in flight");
  g_fix=true; step(1000); click(); step(2600); expect(scr==Scr::Result && lastResult<0,"distance unknown when start had no fix");
  click();
  // Undo last shot
  uint16_t before=round_.h.shotCount;
  hold(); pick("Undo last shot"); turn(1); click(); expect(round_.h.shotCount==before-1,"undo last shot");
  // Go to hole 18, putt, hole out, finish
  hold(); pick("Go to hole..."); expect(scr==Scr::Holes,"hole picker");
  turn(16); screen(); click(); expect(round_.h.currentHole==18,"jumped to hole 18");
  turn(putterIndex()-dialSel); click(); step(2600); turn(1); click();
  expect(scr==Scr::Confirm,"hole 18 hole-out asks to finish");
  screen();
  turn(1); click(); expect(scr==Scr::Home && !round_.active(),"round finished, back home");
  expect(storage::currentRoundId()==0,"no current round pointer after finish");
  screen();
  pick("New round"); click(); step(2600); walkYards(222); screen(); hold(); pick("Scorecard"); screen(); click(); pick("GPS status"); screen(); click();
  pick("End round"); screen(); turn(1); click(); expect(scr==Scr::Home,"second round ended");
  // delete a saved round from the device
  expect(countRounds()==2,"two saved rounds");
  pick("Saved rounds (2)"); screen(); expect(scr==Scr::Menu && menu[1].arg==2,"rounds list, newest first");
  turn(1); click(); expect(scr==Scr::Confirm && confirmSel==0,"delete asks to confirm, defaults to No");
  screen(); click(); expect(scr==Scr::Menu && countRounds()==2,"No keeps the round");
  click(); turn(1); click(); expect(countRounds()==1 && !LittleFS.exists("/rounds/2.bin"),"round 2 deleted");
  expect(scr==Scr::Menu && menuCount==2 && menu[1].arg==1,"list refreshed after delete");
  hold(); expect(scr==Scr::Home && !strcmp(menu[0].label,"New round"),"long press back to a rebuilt home menu");
  // Scorecard of a new round, then export the first round
  Serial.in="ls\ngeojson 1\n"; step(50);
  printf("---- serial ----\n%s", Serial.out.c_str());
  size_t p=Serial.out.find("{\"type\""); std::string js=Serial.out.substr(p); js=js.substr(0,js.find('\n'));
  FILE* f=fopen("round1.geojson","w"); fputs(js.c_str(),f); fclose(f);
  Serial.out.clear(); Serial.in="csv 1\n"; step(50); printf("%s",Serial.out.c_str());
#if BOARD_FEATHER_REVTFT
  // Feather hardware: boot commands, NeoPixel, button repeat, partial screen pushes
  expect(Serial1.out.find("$PMTK251,57600*2C\r\n")==0 && Serial1.baud==GPS_BAUD,"GPS switched to 57600 baud");
  expect(Serial1.out.find("$PMTK314,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n")!=std::string::npos,"GPS set to RMC+GGA+GSA");
  expect(Serial1.out.find("$PMTK220,250*29\r\n")!=std::string::npos,"GPS set to 4 Hz");
  expect(g_panelErrors==0,"every TFT write inside the panel");
  pick("New round"); expect(g_led==0x002800,"NeoPixel green when ready");
  expect(labs((long)round_.h.startUnix-(long)(1790000000+millis()/1000))<=1,"round start time decoded from NMEA date");
  { uint8_t before=dialSel; g_pins[PIN_BTN_DOWN]=HIGH; step(1000); g_pins[PIN_BTN_DOWN]=LOW; step(40);
    expect(dialSel-before>=5,"holding down auto-repeats"); }
  step(1000); g_panelRowsPushed=0; step(1000); expect(g_panelRowsPushed==0,"unchanged screen sends nothing to the TFT");
  g_fix=false; step(3500); g_panelRowsPushed=0; step(1000);
  expect(g_panelRowsPushed>0 && g_panelRowsPushed<=2*2*9,"blinking GPS pin re-sends only header rows");
  g_fix=true; step(500);
  hold(); pick("End round"); turn(1); click(); expect(scr==Scr::Home,"third round ended");
  Serial1.out.clear();
  try { pick("Power off"); } catch(int) { expect(g_slept,"power off enters deep sleep"); }
  expect(Serial1.out=="$PMTK161,0*28\r\n" && g_panelSleep && g_led==0,"GPS standby, TFT asleep, NeoPixel off");
#else
  // power off
  try { pick("Power off"); } catch(int) { expect(g_slept,"power off enters deep sleep"); }
#endif
  printf("\n%d failures\n", fails);
  return fails;
}
