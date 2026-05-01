# 🏗️ ClawMagotchi Architecture

Technical deep-dive into the ClawMagotchi firmware architecture (v0.3).

---

## System Overview

```
┌──────────────┐     HTTP/JSON      ┌──────────────────┐
│  Clawpilot   │──POST──►           │                  │
│  (Windows)   │         ┌──────────┤  Bridge Server   │
└──────────────┘         │  :8222   │  (Python, no deps│
                         │          └──────────────────┘
                         │                  ▲
                         │                  │ GET /api/notifications
                         │                  │ (every 5s)
                         │          ┌───────┴──────────┐
                         │          │   ESP32 Device    │
                         │          │  (ClawMagotchi)   │
                         │          └───────┬──────────┘
                         │                  │
                         │                  │ SPI
                         │                  ▼
                         │          ┌──────────────────┐
                         │          │  TFT LCD Screen   │
                         │          │ 172×320 / 240×320 │
                         │          └──────────────────┘
                         │
  Serial USB ────────────┘  (alternative input path)
```

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

### Conditional Compilation Pattern

```c
#ifdef PLATFORM_WAVESHARE
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <ArduinoJson.h>
  // WiFi-specific code
#else
  // Wokwi fallback (no WiFi, simulated time)
#endif
```

---

## Main Loop Architecture

The `loop()` runs independent subsystems at different rates with no blocking delays:

```
loop() — runs continuously (no delay())
  │
  ├─ seg7_renderTime(h, m, colonOn)    every 500ms    (7-segment digits + colon blink)
  ├─ seg7_renderSec(s)                 every 1000ms   (seconds text)
  ├─ seg7_renderDate(str)              every 60s      (date below clock)
  ├─ tickLobster()                     every 83ms     (~12 fps animation)
  ├─ renderScenarioCard(scenario)      on change      (notification card)
  ├─ pollNotifications()               every 5s       (WiFi HTTP GET)
  ├─ checkSerial()                     every loop     (USB notification input)
  └─ dismissNotification()             auto 8s        (clear card, next in queue)
```

All timing uses `millis()` comparisons — never `delay()`.

---

## 7-Segment Clock Renderer (`segments.h`)

### Overview

A procedural 7-segment display renderer that draws digits using `fillRoundRect()` calls. No bitmaps — pure geometry. This gives a classic green LCD clock aesthetic.

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

Example: digit `0` = `0b0111111` (segments a,b,c,d,e,f on — all except g)

### Digit Lookup Table

```c
static const uint8_t SEG_MAP[10] PROGMEM = {
  0b0111111, // 0
  0b0000110, // 1
  0b1011011, // 2
  0b1001111, // 3
  0b1100110, // 4
  0b1101101, // 5
  0b1111101, // 6
  0b0000111, // 7
  0b1111111, // 8
  0b1101111, // 9
};
```

### Sizing (Adaptive)

The renderer adapts to screen width:

| Parameter | 172px (Waveshare) | 240px (Wokwi) |
|-----------|-------------------|----------------|
| Digit width (`SEG_DW`) | 22px | 30px |
| Digit height (`SEG_DH`) | 38px | 50px |
| Segment thickness (`SEG_TH`) | 4px | 5px |
| Gap between digits (`SEG_GAP`) | 4px | 6px |
| Y origin (`SEG_Y0`) | 44px | 40px |

### Colors

| Color | Hex | RGB565 | Purpose |
|-------|-----|--------|---------|
| SEG_ON | #00FF88 | `0x07F1` | Active segment (bright green) |
| SEG_OFF | — | `0x0220` | Ghost segment (very dim green) |
| SEG_BG | #1A1A2E | `0x18C5` | Background |

### Public API

```c
void seg7_init();                              // No-op (stateless)
void seg7_renderTime(int h, int m, bool colonOn);  // HH:MM with colon
void seg7_renderSec(int s);                    // Seconds below clock
void seg7_renderDate(const char* dateStr);     // Date below seconds
```

### Rendering Details

- Each digit is drawn by iterating 7 segments and calling `fillRoundRect()`
- OFF segments are drawn in dim green (ghost effect, like a real LCD)
- Colon uses two filled circles that toggle between bright/dim every 500ms
- The entire clock area is cleared before redraw (partial update per digit region)

---

## Sprite Composition System (`sprites.h`)

### Overview

A minimal sprite system using 16×16 1-bit bitmaps stored in Flash (PROGMEM). Each icon is 32 bytes — 97% smaller than a full-color 16-bit RGB565 bitmap (512 bytes).

### The 16 Icons

| Icon | Constant | Visual | Used In |
|------|----------|--------|---------|
| ✉️ Envelope | `ICON_ENVELOPE` | Rectangle with V-flap | Email scenario |
| 💬 Chat | `ICON_CHAT` | Rounded rect with tail | Teams scenario |
| 🕒 Clock | `ICON_CLOCK` | Circle with hour/min hands | Meeting, Long Session |
| ⚠️ Warning | `ICON_WARNING` | Triangle with exclamation | Late, Context Switch, Stuck |
| ✅ Check | `ICON_CHECK` | Checkmark shape | Done scenario |
| ❓ Question | `ICON_QUESTION` | Question mark | Task Nudge, Needs Input |
| 💧 Droplet | `ICON_DROPLET` | Teardrop shape | Hydrate |
| 🏃 Runner | `ICON_RUNNER` | Stick figure running | Move |
| 🛡️ Shield | `ICON_SHIELD` | Shield silhouette | Focus |
| 🌅 Sunrise | `ICON_SUNRISE` | Half circle with rays | Morning |
| 🌙 Moon | `ICON_MOON` | Crescent | Shutdown |
| ⚙️ Gear | `ICON_GEAR` | Circle with teeth | Working |
| ⚖️ Scales | `ICON_SCALES` | Balance beam | Decision |
| 🎉 Star | `ICON_STAR` | 5-pointed star | Celebration |
| ↩️ Reply | `ICON_REPLY` | Curved arrow left | Worth Reply |
| 👀 Eye | `ICON_EYE` | Eye shape | Quiet, Doomscroll |

### Bitmap Format

- **Dimensions**: 16×16 pixels (fixed)
- **Color depth**: 1-bit (on/off)
- **Storage**: 2 bytes per row × 16 rows = 32 bytes per icon
- **Byte order**: MSB-first (leftmost pixel = bit 7 of first byte)
- **Storage location**: PROGMEM (Flash memory, not RAM)

### Memory Math

| Approach | Size per icon | 16 icons total |
|----------|--------------|----------------|
| 1-bit PROGMEM (our approach) | 32 B | 512 B |
| 16-bit RGB565 full color | 512 B | 8,192 B |
| **Savings** | **94% less** | **7,680 B saved** |

### Drawing Functions

```c
// Draw icon with transparent background (only ON pixels drawn)
void drawIcon16(int x, int y, const uint8_t* icon, uint16_t color);

// Draw icon with circular badge background (notification style)
void drawIconBadge(int x, int y, const uint8_t* icon, uint16_t iconColor, uint16_t bgColor);
```

### How drawIcon16 Works

```c
for each row (0..15):
    read 2 bytes from PROGMEM → 16 bits
    for each column (0..15):
        if bit is set → drawPixel(x+col, y+row, color)
```

Only "on" pixels are drawn, making the icon transparent against any background.

### Sprite Composition

The lobster can "hold" items in its claws. When `ScenarioConfig.hasHeldItem == true`:
1. The lobster's claw position is calculated based on its current animation frame
2. `drawIcon16()` is called at the claw offset position
3. The icon appears to be held by the lobster

---

## Scenario State Machine (`scenarios.h`)

### Overview

A data-driven state machine that maps 23 distinct situations into lobster behaviors. Each scenario is defined by a `ScenarioConfig` struct in a lookup table.

### Enum Definition

```c
enum Scenario {
  SC_IDLE = 0,        SC_FOCUS,          SC_CONTEXT_SWITCH,
  SC_TASK_NUDGE,      SC_LONG_SESSION,   SC_MEETING_SOON,
  SC_LATE,            SC_EMAIL,          SC_TEAMS_MSG,
  SC_WORTH_REPLY,     SC_QUIET_TOO_LONG, SC_MOVE,
  SC_HYDRATE,         SC_MORNING,        SC_SHUTDOWN,
  SC_WORKING,         SC_DONE,           SC_NEEDS_INPUT,
  SC_STUCK,           SC_DECISION,       SC_DOOMSCROLL,
  SC_CELEBRATION,     SC_IDLE_PERSONALITY,
  SC_COUNT  // sentinel (= 23)
};
```

### Config Struct

```c
struct ScenarioConfig {
  const char* message;       // card text (NULL = no card)
  const uint8_t* icon;       // PROGMEM icon bitmap (NULL = none)
  uint16_t iconColor;        // icon tint
  uint16_t accentColor;      // card left-bar color
  LMood lobsterMood;         // IDLE, HAPPY, ALERT, URGENT
  bool hasHeldItem;          // lobster holds icon in claw?
  const uint8_t* heldIcon;   // which icon to hold (NULL = none)
};
```

### Lookup

```c
const ScenarioConfig& getScenario(Scenario sc);  // O(1) array lookup
```

### Card Rendering

`renderScenarioCard(Scenario sc)` draws:
1. Dark rounded rect background
2. Colored accent bar on left edge
3. Icon badge (circular background + 16×16 icon)
4. Message text (size 2, white on dark)

Card position adapts to screen size (below the 7-segment clock area).

### Mood Integration

The `LMood` enum affects the lobster's animation:

| Mood | Effect on Lobster |
|------|-------------------|
| `IDLE` | Normal roaming speed |
| `HAPPY` | Celebratory bounce, faster movement |
| `ALERT` | Faster movement, looks toward card |
| `URGENT` | Agitated, red glow effect |

---

## Lobster Animation System

### State Machine

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

### Rendering Details

- **Frame rate**: ~12 fps (83ms per frame)
- **Erase method**: Bounding box of previous frame → fill with background → redraw ground dots
- **Body parts**: Shell (ellipse), tail fan, eye, arm+claw (directional), antennae (wave sine), legs (walk cycle)
- **Movement**: 0.8 px/frame walk speed, bounces at screen edges
- **Composition**: Body parts composited each frame (procedural, not sprites)
- **Held items**: When a scenario has `hasHeldItem`, the icon is drawn at the claw position

---

## Notification System

### Data Flow

```
Clawpilot API  ──POST──►  Bridge Server  ◄──GET──  pollNotifications()
                          (holds queue)              │
                                              parseNotifications()
                                                    │
Serial USB     ──readLine──────────────────►  checkSerial()
                                                    │
                                              ┌─────▼─────┐
                                              │  notifs[]  │  ring buffer (max 5)
                                              └─────┬─────┘
                                                    │
                                              scenario mapping
                                                    │
                                              ┌─────▼─────┐
                                              │ Scenario   │  renderScenarioCard()
                                              │ Card + Icon│  + lobster mood change
                                              └─────┬─────┘
                                                    │
                                              auto-dismiss (8s)
                                                    │
                                              next in queue or back to idle
```

### Notification Card Layout (v0.3)

```
┌─────────────────────────────┐
│▌ [●] Message text           │  ← accent bar + icon badge + text
└─────────────────────────────┘
```

- Accent bar: 3px wide, scenario's accent color
- Icon badge: 20px circle with 16×16 icon inside
- Message: size 2 text, white on dark background

### Type Colors

| Type | Accent Color | RGB565 | Used by Scenarios |
|------|-------------|--------|-------------------|
| Blue | `C_NOTIF` | `0x2C9F` | Email, Meeting, Worth Reply |
| Purple | `C_TEAMS` | `0x541F` | Teams Message |
| Orange | `C_WARN` | `0xFD20` | Late, Long Session, Move, Stuck |
| Green | `C_GREEN` | `0x07E0` | Focus, Done, Celebration |
| Gold | `C_GOLD` | `0xFE8C` | Task Nudge, Hydrate, Morning, Decision |
| Gray | `C_GRAY` | `0x7BEF` | Idle, Quiet, Shutdown, Working |
| Red | `C_RED` | `0xE1C8` | Late (urgent) |

### Serial Protocol

Send JSON objects via Serial at 115200 baud:

```json
{"type":"email","title":"Chris Luce","body":"Budget review ready"}
{"type":"urgent","title":"Meeting NOW","body":"Room changed to 4B-201"}
```

Commands:
- `dismiss` — clear current notification
- `status` — print device state

---

## WiFi Management

### Connection Flow

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

### API Polling

- **Endpoint**: `http://<API_HOST>:<API_PORT>/api/notifications`
- **Interval**: 5 seconds (configurable `POLL_INTERVAL` in config.h)
- **Timeout**: 3 seconds per request
- **On error**: Silent retry next cycle (no UI disruption)
- **On success**: Queue is returned and cleared server-side

---

## Memory Layout

| Component | Flash | RAM | Notes |
|-----------|-------|-----|-------|
| Sketch code | ~30 KB | — | Main firmware logic |
| Sprite icons (16×) | 512 B | 0 B | PROGMEM (32 B each) |
| Segment map (10 digits) | 10 B | 0 B | PROGMEM |
| Scenario table (23×) | ~700 B | ~700 B | Pointers prevent PROGMEM |
| Notification queue (5×) | — | ~1.1 KB | 5 × ~220 bytes |
| Lobster state | — | ~40 B | Position, frame, mood |
| Clock state | — | ~30 B | Time, blink flag, date buf |
| String literals | ~500 B | 0 B | PROGMEM (messages) |
| WiFi stack | — | ~40 KB | ESP-IDF managed |
| TFT frame buffer | 0 B | 0 B | No buffer — direct SPI writes |
| **Total** | **~32 KB** | **~42 KB** | ESP32-C6: 4MB Flash / 320KB SRAM |

---

## Screen Layout (320px tall)

```
Y=0    ┌─────────────────────┐
       │                     │
Y~40   │    ██  ██ : ██  ██  │  ← 7-segment clock (HH:MM)
       │       ██            │  ← seconds
       │    Mon, Jun 15      │  ← date (gray, small)
       │                     │
Y~130  │  ┌─────────────────┐│  ← scenario card
       │  │▌[●] Message     ││    (accent bar + icon badge + text)
       │  └─────────────────┘│
       │                     │
Y~240  │  ● online           │  ← WiFi status dot
       │                     │
Y~286  │ ··················· │  ← ground line (dots)
       │     🦞              │  ← lobster (procedural)
Y=310  │              v0.3   │  ← version label
Y=320  └─────────────────────┘
```

---

## File Structure

```
red-device/
├── sketch.ino          # Main firmware — loop, rendering, WiFi, serial
├── config.h            # Platform selection, WiFi, API, pin mappings
├── segments.h          # 7-segment clock renderer (procedural, no bitmaps)
├── sprites.h           # 16×16 1-bit PROGMEM icons (16 icons, 512B total)
├── scenarios.h         # 23-scenario state machine (enum, config, renderer)
├── diagram.json        # Wokwi wiring diagram (ESP32 + ILI9341)
├── libraries.txt       # Wokwi library dependencies
├── bridge/
│   ├── server.py       # HTTP bridge server (pure Python 3 stdlib)
│   ├── push.py         # CLI push tool + importable function
│   ├── test_push.py    # Batch test (4 sample notifications)
│   └── README.md       # Bridge API documentation
├── exampleImages/      # Screenshots and photos
│   ├── Designer.png
│   └── wokwi-v02-running.png
├── README.md           # Project overview
├── ARCHITECTURE.md     # This file
├── SCENARIOS.md        # Scenario system documentation
└── SPRITES.md          # Icon/sprite reference & creation guide
```

---

## Dependencies

| Library | Version | Platform | Purpose |
|---------|---------|----------|---------|
| Adafruit GFX | Latest | Both | Graphics primitives (text, shapes, pixels) |
| Adafruit ILI9341 | Latest | Wokwi | Display driver for emulator |
| Adafruit ST7789 | Latest | Waveshare | Display driver for real hardware |
| ArduinoJson | v7+ | Waveshare | HTTP response parsing |

Bridge server: **Python 3.6+** (stdlib only — `http.server`, `json`, `urllib`)

---

## Design Principles

1. **No blocking** — everything timer-driven with `millis()`
2. **Memory-first** — PROGMEM for all static data, minimal RAM allocation
3. **Single codebase** — `#ifdef` for platform differences, shared logic
4. **Data-driven** — scenarios defined as table data, not scattered if/else
5. **Partial updates** — only redraw changed regions (no full-screen refresh)
6. **Zero external deps for bridge** — Python stdlib only, runs anywhere
7. **Graceful degradation** — WiFi failure doesn't crash, serial always works

---

## Future Architecture Plans

- **v0.4** — Custom pixel art sprites (replacing some procedural elements)
- **v0.5** — Button/touch input → interaction with notification cards
- **v0.6** — Home Assistant MQTT integration (alongside HTTP bridge)
- **v1.0** — Persistent personality state (NVS), stats tracking, growth system

---

*Architecture doc maintained by Red 🦞*
