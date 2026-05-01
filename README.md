# 🦞 Red Device — ClawMagotchi v0.2

A physical desk companion powered by ESP32 — an animated lobster that lives on your desk, shows the time, and lights up with live notifications from [Clawpilot](https://github.com/micbou80/clawmagotchi).

![ClawMagotchi concept](https://img.shields.io/badge/status-v0.2_alpha-orange) ![Platform](https://img.shields.io/badge/platform-ESP32--C6_%7C_Wokwi-blue)

## What It Does

- **🕐 Digital clock** — NTP-synced (CET/CEST) with blinking colon, seconds, and date
- **🦞 Red the Lobster** — animated companion that roams the screen with personality states
- **📡 WiFi notifications** — polls a local API bridge for email, Teams, calendar, and urgent alerts
- **💬 Serial interface** — push notifications via USB for testing on any platform
- **🎨 Notification cards** — color-coded by type with auto-dismiss and queue support
- **😊 Mood system** — lobster reacts to notifications (idle → alert → urgent)

## Dual-Platform Architecture

The firmware runs on **two platforms** from the same codebase:

| | **Waveshare ESP32-C6 LCD 1.47"** | **Wokwi Emulator** |
|---|---|---|
| Display | ST7789 (172×320) | ILI9341 (240×320) |
| WiFi | ✅ Full WiFi + NTP | ❌ Simulated clock |
| Notifications | HTTP polling + Serial | Serial only |
| Platform define | `PLATFORM_WAVESHARE` | `PLATFORM_WOKWI` |

Switch platforms by uncommenting one line in `config.h`.

## Quick Start — Wokwi Emulator

1. Go to [wokwi.com](https://wokwi.com)
2. Create a new **ESP32** project (Arduino)
3. Replace the contents of each file:
   - `sketch.ino` → paste from [`sketch.ino`](sketch.ino)
   - `diagram.json` → paste from [`diagram.json`](diagram.json)
   - `libraries.txt` → paste from [`libraries.txt`](libraries.txt)
4. Add `config.h` as a new tab → paste from [`config.h`](config.h)
5. Make sure `PLATFORM_WOKWI` is uncommented in `config.h`
6. Click **▶ Start Simulation**
7. Watch Red roam! 🦞

### Testing Notifications (Serial)

In the Wokwi serial monitor, paste:
```json
{"type":"email","title":"Chris Luce","body":"FY27 budget review ready"}
```

Other commands:
- `dismiss` — clear notification
- `status` — print WiFi/notification state

## Quick Start — Real Hardware

### Hardware Required
- [Waveshare ESP32-C6 LCD 1.47"](https://www.waveshare.com/esp32-c6-lcd-1.47.htm) (172×320 ST7789 IPS)

### Setup
1. Install [Arduino IDE](https://www.arduino.cc/en/software) or PlatformIO
2. Add ESP32-C6 board support (Espressif Arduino Core)
3. Install libraries: `Adafruit GFX`, `Adafruit ST7789`, `ArduinoJson`
4. Open `config.h`:
   - Uncomment `PLATFORM_WAVESHARE`, comment out `PLATFORM_WOKWI`
   - Set `WIFI_SSID` and `WIFI_PASS`
   - Set `API_HOST` to your Clawpilot PC's IP address
5. Flash and go!

### Pin Configuration (Waveshare)

| GPIO | Function |
|------|----------|
| 6    | SPI MOSI |
| 7    | SPI SCLK |
| 5    | SPI MISO |
| 14   | TFT CS   |
| 15   | TFT DC   |
| 21   | TFT RST  |
| 22   | Backlight (PWM) |

## Notification API

The device polls `http://<your-pc>:8222/api/notifications` every 5 seconds.

**Expected JSON response:**
```json
{
  "notifications": [
    {
      "type": "email",
      "title": "Chris Luce",
      "body": "FY27 budget review — need your input by EOD",
      "time": "09:41"
    }
  ]
}
```

**Notification types:**
| Type | Color | Icon | Use case |
|------|-------|------|----------|
| `email` | 🔵 Blue | `@` | Email notifications |
| `teams` | 🟣 Purple | `T` | Teams messages |
| `calendar` | 🟢 Teal | `C` | Meeting reminders |
| `urgent` | 🟠 Orange | `!` | Urgent / action required |

Empty array `{"notifications": []}` = no notifications (lobster stays idle).

## Color Palette

| Color | Hex | RGB565 | Usage |
|-------|-----|--------|-------|
| Dark Navy | #1A1A2E | `0x18C5` | Background |
| Lobster Red | #E63946 | `0xE1C8` | Body |
| Shadow Red | #C1121F | `0xC083` | Legs, belly |
| Claw Gold | #FFD166 | `0xFE8C` | Claws, antennae |
| White | #FFFFFF | `0xFFFF` | Clock, eyes |

## File Structure

```
red-device/
├── config.h          # Platform selection, WiFi, API, pin config
├── sketch.ino        # Main firmware (clock, lobster, notifications)
├── diagram.json      # Wokwi wiring diagram
├── libraries.txt     # Wokwi library dependencies
├── ARCHITECTURE.md   # Technical deep-dive
└── README.md         # You are here
```

## Roadmap

- [x] v0.1 — Idle clock + animated lobster (Wokwi only)
- [x] v0.2 — Dual-platform architecture, WiFi, NTP, notification system
- [ ] v0.3 — Clawpilot API bridge server + real-time push
- [ ] v0.4 — UI redesign (custom pixel art, themed screens)
- [ ] v0.5 — Mood evolution + interaction (buttons, touch)
- [ ] v1.0 — Full ClawMagotchi with personality, stats, and growth

## License

MIT — do whatever you want with it 🦞

---
*Built with ❤️ by Red 🦞 and Michel*
