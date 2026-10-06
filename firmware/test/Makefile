# Host-side simulation of the firmware: stubs out Arduino + SparkFun/Adafruit libraries,
# feeds fake GPS positions and dial (or button) events, and plays a round through the real
# state machine + storage code. Runs once per board build. Run: make -C firmware/test
# AddressSanitizer hangs at startup on recent macOS, so it's Linux-only.
SAN := $(if $(filter Darwin,$(shell uname -s)),-fsanitize=undefined,-fsanitize=address,undefined)
CXX_SIM = g++ -std=c++17 -O0 -g $(SAN) -Istubs -I../shot_tracker

all: sim sim_feather
sim: sim.cpp ../shot_tracker/*.cpp ../shot_tracker/*.h stubs/*.h
	$(CXX_SIM) -o sim sim.cpp
	./sim
# Adafruit ESP32-S3 Reverse TFT Feather + Ultimate GPS FeatherWing
sim_feather: sim.cpp ../shot_tracker/*.cpp ../shot_tracker/*.h stubs/*.h
	$(CXX_SIM) -DARDUINO_ADAFRUIT_FEATHER_ESP32S3_REVTFT -o sim_feather sim.cpp
	./sim_feather
clean:
	rm -f sim sim_feather round1.geojson
