# 🏗️ ClawMagotchi Architecture

Technical deep-dive into the ClawMagotchi firmware architecture.

## System Overview

```
┌──────────────┐     HTTP/JSON      ┌──────────────────┐
│  Clawpilot   │◄──────────────────►│   ESP32 Device   │
│  (Windows)   │   :8222/api/notif  │  (ClawMagotchi)  │
└──────┬───────┘                    └──────┬───────────┘
       │                                   │
       │  Teams/Email/Calendar             │  ST7789 / ILI9341
       │  monitoring                       │  display output
       ▼                                   ▼
┌──────────────┐                    ┌──────────────────┐
│ Microsoft 365│                    │  172×320 / 240×320│
│   Graph API  │                    │  TFT LCD Screen  │
└──────────────┘                    └──────────────────┘
```

## Dual-Platform Design

The codebase compiles for two targets from a single source:

### Compile-Time Platform Selection

```c
// config.h — uncomment ONE:
// #define PLATFORM_WAVESHARE    // Real hardware
#define PLATFORM_WOKWI           // Emulator
```

### What Changes Per Platform

| Component | Waveshare (real) | Wokwi (emulator) |
|-----------|-----------------|-------------------|
| Display driver | `Adafruit_ST7789` | `Adafruit_ILI9341` |
| Screen width | 172px | 240px |
| WiFi | Full `WiFi.h` stack | Not available |
| HTTP client | `HTTPClient.h` | Not available |
| JSON parser | `ArduinoJson` | Manual string parse |
| NTP time | `configTime()` + `pool.ntp.org` | Simulated from `millis()` |
| Notifications | HTTP poll + Serial | Serial only |

### Conditional Compilation Pattern

```c
#ifdef PLATFORM_WAVESHARE
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <ArduinoJson.h>
  // ... WiFi-specific code
#else
  // Wokwi fallback (no WiFi, lighter JSON parsing)
#endif
```

## Main Loop Architecture

The `loop()` runs four independent subsystems at different rates:

```
loop() — runs continuously
  │
  ├─ tickClock()          every 500ms   (colon blink + time update)
  ├─ tickLobster()        every 83ms    (~12 fps animation)
  ├─ pollNotifications()  every 5s      (WiFi only — HTTP GET)
  ├─ checkSerial()        every loop    (USB notification input)
  └─ dismissNotification() auto         (8s timeout per card)
```

No blocking delays — everything is timer-driven with `millis()`.

## Clock System

### Time Sources (priority order)

1. **NTP** (Waveshare only) — `configTime()` with `pool.ntp.org`, CET/CEST auto-adjust
2. **Simulated** (fallback) — increments from 12:00:00 using `millis()` delta

### Rendering

- **Hours:Minutes** — large text (size 5–6 depending on screen width)
- **Colon** — blinks every 500ms (dim ↔ bright)
- **Seconds** — small text below clock, updated every second
- **Date** — "Mon, May 1" format, updated once per minute from NTP

Partial screen updates only — no full redraws. Each digit position is tracked and only redrawn when it changes.

## Lobster State Machine

```
         ┌───────────────────────────────────┐
         ▼                                   │
    ┌─────────┐    random     ┌──────────┐   │
    │   SIT   │──────────────►│  GO_RIGHT │──┤ boundary
    │ 3-8 sec │◄──────────┐   │  2-5 sec  │  │ hit
    └────┬────┘           │   └───────────┘  │
         │                │                   │
         │ random         │   ┌──────────┐   │
         │                ├───│  GO_LEFT  │──┘
         │                │   │  2-5 sec  │
         ▼                │   └───────────┘
    ┌─────────┐           │
    │  LOOK   │───────────┘
    │ 1.5-3s  │
    └─────────┘
```

### Mood System

The lobster's mood changes based on notification state:

| Mood | Trigger | Behavior (future) |
|------|---------|-------------------|
| `IDLE` | No notifications | Normal roaming |
| `HAPPY` | Good news / dismiss | Celebratory animation |
| `ALERT` | New notification | Faster movement, looks at notification |
| `URGENT` | Urgent notification | Agitated, red glow effect |

### Animation Details

- **Frame rate**: ~12 fps (83ms per frame)
- **Erase method**: Bounding box of previous frame → fill with background → redraw ground dots
- **Body parts**: Shell (ellipse), tail fan, eye, arm+claw (directional), antennae (wave), legs (walk cycle)
- **Movement**: 0.8 px/frame, bounces at screen edges

## Notification System

### Data Flow

```
Clawpilot API  ──HTTP GET──►  pollNotifications()
                                    │
                              parseNotifications()
                                    │
Serial USB     ──readLine──►  checkSerial()
                                    │
                              ┌─────▼─────┐
                              │  notifs[]  │  ring buffer (max 5)
                              └─────┬─────┘
                                    │
                              showNotification()
                                    │
                              ┌─────▼─────┐
                              │  TFT card  │  color-coded, auto-dismiss 8s
                              └─────┬─────┘
                                    │
                              dismissNotification()
                                    │
                              next in queue or back to clock
```

### Notification Card Layout

```
┌─────────────────────────────┐
│ ● Title text          09:41 │  ← accent color bar on left
│   Body text line 1          │  ← icon badge (type-specific)
│   Body text line 2          │
│           1/3               │  ← queue position (if multiple)
└─────────────────────────────┘
```

### Type Colors

| Type | Accent Color | RGB565 |
|------|-------------|--------|
| `email` | Blue | `0x2C9F` |
| `teams` | Purple | `0x541F` |
| `calendar` | Teal | `0x04A0` |
| `urgent` | Orange | `0xFD20` |

### Serial Protocol

Send JSON objects via Serial (115200 baud):

```json
{"type":"email","title":"Chris Luce","body":"Budget review ready"}
{"type":"urgent","title":"Meeting NOW","body":"Room changed to 4B-201"}
```

Commands:
- `dismiss` — clear current notification
- `status` — print device state

## WiFi Management

### Connection Flow

```
setup()
  └─ connectWiFi()
       ├─ WiFi.mode(WIFI_STA)
       ├─ WiFi.begin(SSID, PASS)
       ├─ Wait up to 15s
       └─ Success? → configTime(NTP) → drawStatus("online")
                    → drawStatus("offline")

loop()
  └─ if !wifiConnected && 30s elapsed:
       └─ connectWiFi()   // auto-reconnect
```

### API Polling

- Endpoint: `http://<API_HOST>:<API_PORT>/api/notifications`
- Interval: 5 seconds (configurable in `config.h`)
- Timeout: 3 seconds per request
- On error: silent retry next cycle (no UI disruption)

## Memory Layout

| Component | Approximate RAM |
|-----------|----------------|
| TFT frame buffer | 0 (no buffer — direct SPI) |
| Notification array | 5 × ~220 bytes ≈ 1.1 KB |
| Lobster state | ~40 bytes |
| Clock state | ~30 bytes |
| WiFi stack | ~40 KB (ESP-IDF managed) |
| **Total sketch** | ~45 KB (well within ESP32-C6's 320 KB SRAM) |

## Screen Layout (320px tall)

```
Y=0    ┌─────────────────────┐
       │                     │
Y=55   │     12:34           │  ← clock (large)
       │       :05           │  ← seconds (small)
       │   Mon, May 1        │  ← date
       │                     │
Y~160  │  ┌─notification──┐  │  ← card (when active)
       │  │ @ Title  09:41│  │
       │  │   Body text   │  │
       │  └───────────────┘  │
       │                     │
Y~240  │  ● online           │  ← status dot
       │                     │
Y~286  │ ··················· │  ← ground line
       │     🦞              │  ← lobster
Y=310  │              v0.2   │  ← version
Y=320  └─────────────────────┘
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| Adafruit GFX | Latest | Graphics primitives |
| Adafruit ILI9341 | Latest | Wokwi display driver |
| Adafruit ST7789 | Latest | Waveshare display driver |
| ArduinoJson | v7+ | HTTP response parsing (Waveshare only) |

## Future Architecture Plans

- **v0.3** — Clawpilot notification bridge server (Python, runs on PC)
- **v0.4** — Pixel art sprites replacing procedural drawing
- **v0.5** — Button/touch input for notification interaction
- **v1.0** — Persistent personality state (EEPROM/NVS), stats tracking

---
*Architecture doc maintained by Red 🦞*
