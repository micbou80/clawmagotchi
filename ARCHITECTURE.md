# 🏗️ ClawMagotchi Architecture

Technical deep-dive into the ClawMagotchi firmware architecture (v0.3).

---

## System Overview

```
┌──────────────┐     HTTP/JSON      ┌──────────────────┐
│  Clawpilot   │──POST──►           │                  │
│  (Windows)   │         ┌──────────┤  Bridge Server   │
└──────────────┘         │  :8222   │  (Python, no deps)│
                         │          └──────────────────┘
                         │                  ▲
                         │                  │ GET /api/notifications
                         │                  │ (every 5s)
                         │          ┌───────┴──────────┐
                         │          │   ESP32-C6       │
                         │          │  (ClawMagotchi)   │
                         │          └───────┬──────────┘
                         │                  │
                         │                  │ SPI
                         │                  ▼
                         │          ┌──────────────────┐
                         │          │  ST7789 LCD       │
                         │          │  172×320 px       │
                         │          └──────────────────┘
                         │
  Serial USB ────────────┘  (alternative input path)
```

### Clawpilot Integration

Clawpilot's heartbeat automation monitors M365 signals and pushes notifications:

- **Email** — important emails trigger `email` type
- **Teams** — messages trigger `teams` type
- **Calendar** — upcoming meetings trigger `calendar` with `--countdown` seconds
- **Working** — when Copilot agent is active, pushes `working` type (lobster types at laptop)
- **Working Done** — agent completes, pushes `working_done` (celebratory bounce)

The bridge auto-starts on Windows login via a shortcut in the Startup folder.

---

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
| JSON parser | `ArduinoJson` v7 | Manual string parse |
| NTP time | `configTime()` + `pool.ntp.org` | Simulated from `millis()` |
| Notifications | HTTP poll + Serial | Serial only |
| Segment sizing | 22px digits | 30px digits |

---

## Main Loop Architecture

The `loop()` runs independent subsystems at different rates with no blocking delays:

```
loop() — runs continuously (no delay())
  │
  ├─ seg7_renderTime(h, m, colonOn)    every 500ms    (7-segment digits + colon blink)
  ├─ seg7_renderDate(str)              every 60s      (date below clock)
  ├─ tickLobster()                     every 83ms     (~12 fps animation)
  ├─ renderScenarioCard(scenario)      on change      (notification card)
  ├─ renderCountdown(seconds)          every 1s       (MM:SS countdown when active)
  ├─ renderWorkingState()              every 200ms    (laptop + code dot animation)
  ├─ pollNotifications()               every 5s       (WiFi HTTP GET)
  ├─ checkSerial()                     every loop     (USB notification input)
  └─ dismissNotification()             auto 8s        (clear card, next in queue)
```

All timing uses `millis()` comparisons — never `delay()`.

---

## V2 Lobster System (`lobster_gfx.h`)

### Expressions

The V2 lobster has procedurally drawn expressions that change with mood:

| Expression | Visual | Trigger |
|-----------|--------|---------|
| Normal | Cyan eyes, gentle antenna sway | Idle, info |
| Happy | Wide eyes, bouncy walk | Celebration, done |
| Alert | Focused eyes, fast antenna | New notification |
| Sleepy | Half-closed eyes, slow droop | Long session |
| Dizzy | Spiral eyes, wobble | Context switching |

### Claw Poses

| Pose | Description | When Active |
|------|-------------|-------------|
| Idle | Resting at side | Default |
| Raised | Lifted up | Alert scenarios |
| Holding | Gripping an icon | Email (envelope), Teams (chat), etc. |
| Tucked | Folded under body | Sleepy, shutdown |
| Typing | Tapping at tiny laptop | Working state |

### Working State Animation

When `working` type is received:
1. A tiny laptop sprite appears on the ground
2. The lobster positions behind it in the **typing** claw pose
3. Animated dots scroll across the laptop screen (simulating code)
4. `working_done` clears the laptop and triggers a celebratory bounce

---

## Countdown Timer

Activated by sending a `countdown` field (seconds) with a notification:

```bash
python push.py --type calendar --title "Standup" --body "Starting" --countdown 300
```

### Rendering

- Large MM:SS digits rendered with the 7-segment system
- Replaces the notification card area while active
- Color shifts based on remaining time:

| Time Remaining | Color |
|---------------|-------|
| > 60 s | White |
| ≤ 60 s | Red |
| 0 s | Flashing red/white |

---

## 7-Segment Clock Renderer (`segments.h`)

### Overview

A procedural 7-segment display renderer that draws digits using `fillRoundRect()` calls. No bitmaps — pure geometry. In v0.3 the digits are **white** (bigger, no seconds display) for a cleaner look.

### Segment Mapping

```
   ─── a ───
  |         |
  f         b
  |         |
   ─── g ───
  |         |
  e         c
  |         |
   ─── d ───
```

Each digit is encoded as a 7-bit value where bit positions map to segments:

| Bit | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|
| Seg | g | f | e | d | c | b | a |

### Sizing (Adaptive)

| Parameter | 172px (Waveshare) | 240px (Wokwi) |
|-----------|-------------------|----------------|
| Digit width (`SEG_DW`) | 22px | 30px |
| Digit height (`SEG_DH`) | 38px | 50px |
| Segment thickness (`SEG_TH`) | 4px | 5px |
| Gap between digits (`SEG_GAP`) | 4px | 6px |

### Public API

```c
void seg7_init();
void seg7_renderTime(int h, int m, bool colonOn);  // HH:MM with colon blink
void seg7_renderDate(const char* dateStr);          // date below clock
```

---

## Sprite Composition System (`sprites.h`)

A minimal sprite system using 16×16 1-bit bitmaps stored in Flash (PROGMEM). Each icon is 32 bytes — 94% smaller than full-color RGB565.

### Drawing Functions

```c
void drawIcon16(int x, int y, const uint8_t* icon, uint16_t color);
void drawIconBadge(int x, int y, const uint8_t* icon, uint16_t iconColor, uint16_t bgColor);
```

Only "on" pixels are drawn — icons are transparent against any background.

---

## Scenario State Machine (`scenarios.h`)

A data-driven state machine mapping 23 distinct situations into lobster behaviors. Each scenario is defined by a `ScenarioConfig` struct.

### Config Struct

```c
struct ScenarioConfig {
  const char* message;       // card text (NULL = no card)
  const uint8_t* icon;       // PROGMEM icon bitmap
  uint16_t iconColor;        // icon tint
  uint16_t accentColor;      // card accent color
  LMood lobsterMood;         // IDLE, HAPPY, ALERT, URGENT
  bool hasHeldItem;          // lobster holds icon in claw?
  const uint8_t* heldIcon;   // which icon to hold
};
```

### Notification Card Layout (v0.3)

Cards are centered with no accent bar or type label — just the icon badge and message text on a dark rounded-rect background.

---

## Notification System

### Data Flow

```
Clawpilot/push.py  ──POST──►  Bridge Server  ◄──GET──  pollNotifications()
                               (queue ≤ 10)              │
                                                    parse JSON
                                                         │
Serial USB  ──readLine──────────────────────►  checkSerial()
                                                         │
                                                   ┌─────▼─────┐
                                                   │  notifs[]  │  ring buffer (max 5)
                                                   └─────┬─────┘
                                                         │
                                                   scenario mapping
                                                         │
                                               ┌─────────▼─────────┐
                                               │ Card + Lobster    │  renderScenarioCard()
                                               │ mood + countdown  │  + countdown / working
                                               └─────────┬─────────┘
                                                         │
                                                   auto-dismiss (8s)
```

### Serial Protocol

Send JSON over Serial at 115200 baud:

```json
{"type":"email","title":"Chris Luce","body":"Budget review ready"}
{"type":"working","title":"Agent","body":"Researching..."}
{"type":"calendar","title":"Standup","body":"Starting","countdown":300}
```

Commands: `dismiss`, `status`

---

## WiFi Management

```
setup()
  └─ connectWiFi()
       ├─ WiFi.mode(WIFI_STA)
       ├─ WiFi.begin(SSID, PASS)
       ├─ Wait up to 15s
       └─ Success? → configTime(NTP) → show "online"
                    → show "offline"

loop()
  └─ if !connected && 30s elapsed → reconnect attempt
```

---

## Screen Layout (320px tall)

```
Y=0    ┌─────────────────────┐
       │                     │
Y~40   │    ██  ██ : ██  ██  │  ← 7-segment clock (HH:MM, white)
       │    Mon, Jun 15      │  ← date (gray, small)
       │                     │
Y~130  │   ┌───────────────┐ │  ← notification card (centered)
       │   │  [●] Message  │ │    or countdown MM:SS
       │   └───────────────┘ │    or working state (laptop)
       │                     │
Y~240  │  ● online           │  ← WiFi status dot
       │                     │
Y~286  │ ··················· │  ← ground line (dots)
       │     🦞              │  ← V2 lobster (procedural)
Y=310  │              v0.3   │  ← version label
Y=320  └─────────────────────┘
```

---

## File Structure

```
red-device/
├── sketch/                 # Arduino IDE project folder
│   ├── sketch.ino          # Main firmware
│   ├── config.h            # Platform, WiFi, API, pins
│   ├── segments.h          # 7-segment clock renderer
│   ├── sprites.h           # 16×16 1-bit PROGMEM icons
│   ├── scenarios.h         # 23-scenario state machine
│   ├── scenarios_v2.h      # V2 scenario extensions
│   ├── lobster_gfx.h       # V2 lobster (expressions, poses)
│   └── icons_large.h       # Larger icon variants
├── bridge/
│   ├── server.py           # HTTP bridge (Python 3 stdlib)
│   ├── push.py             # CLI push tool
│   ├── test_push.py        # Batch test
│   └── README.md           # Bridge API docs
├── exampleImages/          # Screenshots + photos
├── diagram.json            # Wokwi wiring diagram
├── libraries.txt           # Wokwi library deps
├── README.md               # Project overview
├── ARCHITECTURE.md         # This file
├── SCENARIOS.md            # Scenario system docs
└── SPRITES.md              # Icon/sprite reference
```

---

## Dependencies

| Library | Platform | Purpose |
|---------|----------|---------|
| Adafruit GFX | Both | Graphics primitives |
| Adafruit ILI9341 | Wokwi | Emulator display driver |
| Adafruit ST7789 | Waveshare | Hardware display driver |
| ArduinoJson v7+ | Waveshare | HTTP response parsing |

Bridge server: **Python 3.6+** (stdlib only — `http.server`, `json`, `urllib`)

---

## Design Principles

1. **No blocking** — everything timer-driven with `millis()`
2. **Memory-first** — PROGMEM for all static data, minimal RAM
3. **Single codebase** — `#ifdef` for platform differences
4. **Data-driven** — scenarios as table data, not if/else chains
5. **Partial updates** — only redraw changed regions
6. **Zero deps for bridge** — Python stdlib only
7. **Graceful degradation** — WiFi failure doesn't crash; serial always works

---

*Architecture doc maintained by Red 🦞*
