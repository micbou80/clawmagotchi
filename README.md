# 🦞 ClawMagotchi

> A physical desk companion powered by ESP32 — an animated lobster that lives on a 1.47" screen, shows the time, and reacts to live notifications from your AI assistant.



---

## Features

- **V2 Lobster** — procedural animated lobster with cyan eyes, 5 expressions (normal, happy, alert, sleepy, dizzy), and 5 claw poses (idle, raised, holding, tucked, typing)
- **7-Segment Clock** — white retro-style digits (HH:MM), colon blink, date line — no seconds for a cleaner look
- **23 Scenario States** — data-driven state machine mapping AI signals to lobster behaviors, moods, icons, and notification cards
- **Working State** — lobster sits behind a tiny laptop with animated scrolling code dots
- **Countdown Timer** — big MM:SS display for meeting countdowns; goes red under 60 s, flashes at 0
- **Notification Bridge** — Python 3 HTTP server (zero dependencies) on port 8222; queues notifications for the device
- **Push Script** — `push.py` with `--type`, `--title`, `--body`, `--countdown` arguments
- **WiFi + NTP** — auto-connects, syncs time from `pool.ntp.org`, reconnects on failure
- **Clawpilot Integration** — heartbeat automation pushes email / Teams / calendar / working alerts automatically
- **Bridge Auto-Start** — Windows Startup shortcut launches the bridge on login
- **Dual Platform** — single codebase compiles for real hardware (Waveshare ST7789) and Wokwi emulator (ILI9341)

---

## Hardware

| Part | Spec |
|------|------|
| Board | [Waveshare ESP32-C6-LCD-1.47"](https://www.waveshare.com/esp32-c6-lcd-1.47.htm) |
| Display | ST7789 IPS, 172 × 320 px |
| Connectivity | WiFi 6 (802.11ax), BLE 5, USB-C |
| Price | ~$15 + USB-C cable |

No wiring required — display and ESP32-C6 are integrated on the board.

---

## Quick Start

### 1. Arduino IDE Setup

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. **File → Preferences** → add board URL:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. **Tools → Board → Board Manager** → install **esp32 by Espressif**

### 2. Install Libraries

**Sketch → Include Library → Manage Libraries:**

| Library | Purpose |
|---------|---------|
| Adafruit GFX Library | Graphics primitives |
| Adafruit ST7789 | TFT display driver |
| ArduinoJson (v7+) | HTTP JSON parsing |

### 3. Configure `config.h`

```c
#define PLATFORM_WAVESHARE        // real hardware
// #define PLATFORM_WOKWI         // emulator

#define WIFI_SSID  "YourNetwork"
#define WIFI_PASS  "YourPassword"

#define API_HOST   "192.168.1.100"  // PC running the bridge
#define API_PORT   8222
```

### 4. Flash

1. Connect via USB-C
2. **Tools → Board** → `ESP32C6 Dev Module`
3. **Tools → Port** → select the COM port
4. Click **Upload**

### 5. Start the Bridge

```bash
cd bridge/
python server.py          # runs on port 8222
```

Test end-to-end:

```bash
python bridge/push.py --type email --title "Hello Red" --body "It works!"
```

---

## Notification Types

| Type | Card Color | Description |
|------|-----------|-------------|
| `email` | Blue | Important email arrived |
| `teams` | Purple | Teams message |
| `calendar` | Blue | Meeting reminder / countdown |
| `urgent` | Red | Time-critical alert |
| `info` | Gold | General information |
| `working` | Gray | Copilot agent is working (shows laptop animation) |
| `working_done` | Green | Agent finished (celebratory bounce) |

### Countdown Timer

```bash
python bridge/push.py --type calendar --title "Standup" --body "Starting" --countdown 300
```

The device shows a large MM:SS timer. Under 60 s the digits turn red; at 0 they flash.

---

## Lobster Animations

| Expression | Trigger |
|-----------|---------|
| Normal | Idle pacing, gentle antenna wave |
| Happy | Celebration, task done — bounce and fast movement |
| Alert | New notification — faster walk, looks toward card |
| Sleepy | Long session — droopy antenna |
| Dizzy | Context switching — wobble animation |

| Claw Pose | When |
|----------|------|
| Idle | Default resting pose |
| Raised | Alert / notification incoming |
| Holding | Carrying an icon (envelope, chat bubble, etc.) |
| Tucked | Sleepy / shutdown |
| Typing | Working state — tapping at tiny laptop |

---

## Architecture

```
┌─────────────┐  POST /api/   ┌──────────────┐  GET /api/   ┌─────────────────────────┐
│  Clawpilot  │──notifications─►│    Bridge    │◄─notifications──│  ESP32-C6 + ST7789 LCD │
│  (Windows)  │                │  (Python 3)  │  (polls 5s)  │      ClawMagotchi       │
└─────────────┘                │  port 8222   │              └─────────────────────────┘
                               └──────────────┘
                                     ▲
                                     │ push.py / curl / automation
```

- **Clawpilot heartbeat** monitors email, Teams, calendar → pushes scenario notifications
- **Bridge** queues up to 10 notifications; `GET` returns and clears the queue
- **ESP32** polls every 5 s, maps type → scenario, renders card + lobster mood
- **Serial fallback** — JSON over USB at 115200 baud always works (no WiFi needed)

---

## File Structure

```
red-device/
├── sketch/                 # Arduino IDE project folder
│   ├── sketch.ino          # Main firmware (loop, rendering, WiFi, serial)
│   ├── config.h            # Platform selection, WiFi creds, pin mappings
│   ├── segments.h          # 7-segment clock renderer (procedural)
│   ├── sprites.h           # 16×16 1-bit PROGMEM icons
│   ├── scenarios.h         # 23-scenario state machine
│   ├── scenarios_v2.h      # V2 scenario extensions
│   ├── lobster_gfx.h       # V2 lobster drawing (expressions, claw poses)
│   └── icons_large.h       # Larger icon variants
├── bridge/
│   ├── server.py           # HTTP bridge server (pure Python 3, zero deps)
│   ├── push.py             # CLI push tool (--type, --title, --body, --countdown)
│   ├── test_push.py        # Batch test (4 sample notifications)
│   └── README.md           # Bridge API documentation
├── exampleImages/          # Screenshots and hardware photos
├── diagram.json            # Wokwi wiring diagram
├── libraries.txt           # Wokwi library dependencies
├── README.md               # This file
├── ARCHITECTURE.md         # Technical deep-dive
├── SCENARIOS.md            # Scenario system docs
└── SPRITES.md              # Icon/sprite reference
```

---

## Development

### Wokwi Emulator

The firmware also runs on the free [Wokwi ESP32 simulator](https://wokwi.com) — switch platform in `config.h`:

```c
// #define PLATFORM_WAVESHARE
#define PLATFORM_WOKWI
```

Wokwi uses ILI9341 (240×320) and simulated time. Serial notifications work; WiFi/HTTP do not.

### Pin Configuration (Waveshare ESP32-C6 LCD 1.47")

| GPIO | Function |
|------|----------|
| 6 | SPI MOSI |
| 7 | SPI SCLK |
| 5 | SPI MISO |
| 14 | TFT CS |
| 15 | TFT DC |
| 21 | TFT RST |
| 22 | Backlight (PWM) |

---

## Roadmap

- [x] **v0.1** — Idle clock + animated lobster (Wokwi)
- [x] **v0.2** — Dual-platform, WiFi, NTP, notification system
- [x] **v0.3** — V2 lobster, working state, countdown timer, real hardware
- [ ] **v0.4** — Custom pixel art sprites, UI polish
- [ ] **v0.5** — Button/touch interaction
- [ ] **v1.0** — Persistent personality, stats, growth system

---

## License

MIT 🦞

---

*Built with ❤️ by Red 🦞 and Michel*
