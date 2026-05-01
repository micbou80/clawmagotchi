# 🦞 Red Device — ClawMagotchi v0.1

A digital desk companion powered by ESP32-C6 with an animated lobster friend.

## What It Does

- **Digital clock** with blinking colon and seconds display
- **Red the Lobster** roams the bottom of the screen with personality:
  - 🪑 **Sitting** — claws open/close, antennae wave, occasional blink
  - 🚶 **Walking** — legs animate, moves across the screen
  - 👀 **Looking around** — alternates facing direction curiously
- Smooth 12fps animation with efficient partial-screen updates

## Quick Start (Wokwi Emulator)

1. Go to [wokwi.com](https://wokwi.com)
2. Create a new **ESP32-C6** project (Arduino)
3. Replace the contents of each file:
   - `sketch.ino` → paste from `sketch.ino`
   - `diagram.json` → paste from `diagram.json`
   - `libraries.txt` → paste from `libraries.txt`
4. Click **▶ Start Simulation**
5. Watch Red roam! 🦞

## Pin Wiring (Wokwi)

| ESP32-C6 | ILI9341 | Signal |
|----------|---------|--------|
| GPIO 6   | SCK     | SPI Clock |
| GPIO 7   | MOSI    | SPI Data |
| GPIO 10  | CS      | Chip Select |
| GPIO 8   | DC      | Data/Command |
| 3V3      | VCC     | Power |
| GND      | GND     | Ground |

## Color Palette

| Color | Hex | RGB565 | Usage |
|-------|-----|--------|-------|
| Dark Navy | #1A1A2E | 0x18C5 | Background |
| Lobster Red | #E63946 | 0xE1C8 | Body |
| Shadow Red | #C1121F | 0xC083 | Legs, belly |
| Claw Gold | #FFD166 | 0xFE8C | Claws, antennae |
| White | #FFFFFF | 0xFFFF | Clock, eyes |

## Lobster Behavior State Machine

```
    ┌─────────┐
    │   SIT   │◄──── boundary hit
    └────┬────┘
         │ random
    ┌────┴────┐
    │ WALK_R  │───► edge → SIT
    │ WALK_L  │───► edge → SIT
    │  LOOK   │───► done → SIT/WALK
    └─────────┘
```

States are chosen randomly with weighted probabilities:
- **SIT**: 30% chance, lasts 3–8 seconds
- **WALK_R**: 20%, lasts 2–5 seconds
- **WALK_L**: 20%, lasts 2–5 seconds
- **LOOK**: 30%, lasts 1.5–3 seconds

## Adapting for Real Hardware (ST7789)

The emulator uses ILI9341 (240×320). For the actual ESP32-C6-LCD-1.47 (172×320 ST7789):

1. Replace library: `Adafruit_ILI9341` → `TFT_eSPI` or `Adafruit_ST7789`
2. Update `SW` from 240 to 172
3. Adjust SPI pins to match the board's built-in LCD connection
4. The lobster and clock code stays the same!

## Roadmap

- [x] v0.1 — Idle clock + animated lobster
- [ ] v0.2 — WiFi + API bridge integration
- [ ] v0.3 — Notification animations (email, Teams, meeting, urgent)
- [ ] v0.4 — LED matrix sync (8×8 WS2812 glow)
- [ ] v1.0 — Full ClawMagotchi with personality evolution

---
*Built with ❤️ by Red 🦞 for Michel*
