# Feather setup: step by step

Gets open-gps running on an **Adafruit ESP32-S3 Reverse TFT Feather**. Do the steps in order; each ends with a **Check**.

You need: the Feather, a USB-C **data** cable, and a computer running Linux, macOS or Windows. The **Ultimate GPS FeatherWing** and a battery are optional for now. Without the GPS everything works except distances. See [Running without the GPS](#running-without-the-gps).

For the hardware details behind these steps, see [README.md](README.md#adafruit-feather-build).

---

## 1. Install arduino-cli

| OS | Command |
|---|---|
| Linux | `curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \| BINDIR=~/.local/bin sh` |
| macOS | `brew install arduino-cli` |
| Windows | `winget install ArduinoSA.CLI` |

On Linux, make sure `~/.local/bin` is on your `PATH`. Open a new terminal if `arduino-cli` isn't found.

**Check:** `arduino-cli version` prints a version.

## 2. Install the ESP32 core and libraries

Copy and run the whole block. The core download is about 1.5 GB.

```bash
arduino-cli config init --overwrite   # skip if you already use arduino-cli
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Adafruit GPS Library" "Adafruit ST7735 and ST7789 Library" \
  "SparkFun Qwiic OLED Arduino Library" "SparkFun MAX1704x Fuel Gauge Arduino Library"
```

The two SparkFun libraries are needed even though there are no SparkFun parts: one provides the screen fonts, the other reads the Feather's battery chip.

**Check:** `arduino-cli lib list` shows all four libraries (plus Adafruit GFX and BusIO, which install automatically).

## 3. Let your user open the serial port (Linux only)

Serial ports belong to a group, and its name depends on your distro:

| Distro | Command |
|---|---|
| Arch, CachyOS, Manjaro | `sudo usermod -aG uucp $USER` |
| Ubuntu, Debian, Fedora, most others | `sudo usermod -aG dialout $USER` |

**Log out and back in** (or reboot) for it to take effect.

**Check:** `groups` lists `uucp` or `dialout`.

## 4. Put the Feather in upload mode (first time only)

Out of the box the Feather shows up as a USB drive (`FTHRS3BOOT`) or runs Adafruit's demo. Neither accepts an Arduino upload, so start it in the chip's built-in bootloader:

1. Plug in the Feather.
2. **Hold D0** (the top button, also called BOOT).
3. **Press and release RESET** while still holding D0.
4. **Release D0.**

The screen goes blank. That's expected.

**Check:** `arduino-cli board list` shows a port: `/dev/ttyACM0` on Linux, `/dev/cu.usbmodem…` on macOS, `COM…` on Windows. If nothing is listed, try another cable (charge-only cables don't work) and repeat.

After the first upload you can skip this step. Later uploads reset the board for you.

## 5. Build and upload

From the repository root, with the port from step 4:

```bash
PORT=/dev/ttyACM0
arduino-cli compile --upload -p "$PORT" \
  -b "esp32:esp32:adafruit_feather_esp32s3_reversetft:CDCOnBoot=cdc,PartitionScheme=default" \
  firmware/shot_tracker
```

> Using **fish**? Write `set PORT /dev/ttyACM0` instead of `PORT=/dev/ttyACM0`.
>
> On an Apple Silicon Mac without Rosetta, add `--build-property "tools.ctags.path=$PWD/tools/noop-ctags"`.

**Check:** the output ends with `Hash of data verified.` Then **press RESET**. The board doesn't always restart by itself after a first upload from bootloader mode.

## 6. Check it's running

The screen shows **Shot Tracker**, then the home menu. The first boot formats storage, so it can stay blank for a few seconds.

To see what the board found, open the serial monitor. Run `arduino-cli board list` again first, because the port can change name after the first upload.

```bash
arduino-cli monitor -p "$PORT" -c baudrate=115200
```

Press RESET and look for:

```
tft=1 buttons=1 gps=1 fuel=1 fs=1
```

`gps=0` is normal without the GPS wing. `fs=0` means the partition setting is wrong; check that the upload command includes `PartitionScheme=default`. Type `help` for the export commands, and press Ctrl+C to leave.

## 7. Use it

| Button | What it does |
|---|---|
| **D0** (top) | Previous club / move up. Hold to scroll. |
| **D1** (middle) | Select, or mark at the ball |
| **Hold D1** | Open the menu |
| **D2** (bottom) | Next club / move down. Hold to scroll. |

How to play a round is in [README.md](README.md#playing-a-round). Wherever it says "turn the dial", use D0/D2; for "click", press D1.

Two settings in `firmware/shot_tracker/config.h` you may want to change. Re-run step 5 after editing.
- **Up and down feel backwards:** swap the numbers for `PIN_BTN_UP` and `PIN_BTN_DOWN`.
- **Screen is upside down:** change `TFT_ROTATION` from `3` to `1`.

## Running without the GPS

Until the GPS wing is fitted:
- The screen shows "GPS not found!" at boot, and the GPS pin in the header blinks with an `x`.
- When you mark a shot you get **NO GPS FIX** after 2.5 s. Choose **Save without GPS** to keep going. Strokes are counted, but the distance shows as `--`.

## Adding the GPS FeatherWing

1. Unplug USB and the battery.
2. Stack the Feather onto the FeatherWing (or put both on a FeatherWing Doubler). Optionally fit a CR1220 coin cell to the wing; it makes later fixes faster.
3. Plug back in and press RESET. No re-upload is needed.
4. Go outside. The first fix can take 1–3 minutes.

**Check:** the serial monitor shows `gps=1`. Typing `gps` should show `fix=3 ok=1` once it has a fix. The header shows a satellite count instead of `?`.

If the Feather sits on top of the wing, it blocks the wing's built-in antenna. Use an external antenna on the wing's uFL connector. The magnetic SMA antenna works through a uFL-to-SMA adapter.

## Troubleshooting

| Problem | Fix |
|---|---|
| `board list` shows no port | Use a data cable, then do step 4 again |
| `Permission denied` on `/dev/ttyACM0` | Step 3, then log out and back in |
| Upload fails partway | Step 4 again, then upload again |
| Blank screen after upload | Press RESET |
| `fs=0` at boot | Upload with `PartitionScheme=default` (step 5) |
| Want Adafruit's demo or CircuitPython back | Follow Adafruit's *factory reset* guide for the board. It reinstalls their bootloader. |
