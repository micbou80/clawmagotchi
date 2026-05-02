// ═══════════════════════════════════════════
// ClawMagotchi v0.3 — Wokwi Merged Build
// Composited Lobster + Large Icons + Scenarios
// ═══════════════════════════════════════════

// ── CONFIG ──
/*
 * ╔═════════════════════════════════════════╗
 * ║  RED DEVICE — Configuration             ║
 * ║  WiFi, API, Hardware Settings           ║
 * ╚═════════════════════════════════════════╝
 */

// ────────────────────────────────────────────
//  WiFi Configuration
// ────────────────────────────────────────────
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASS     "YOUR_WIFI_PASSWORD"
#define WIFI_TIMEOUT  15000   // ms to wait for connection

// ────────────────────────────────────────────
//  API Bridge Configuration
// ────────────────────────────────────────────
// The device polls this endpoint for notifications.
// Clawpilot runs a tiny HTTP server on Michel's PC.
// Expected JSON response:
//   { "notifications": [
//       { "type": "email|teams|calendar|urgent",
//         "title": "...",
//         "body": "...",
//         "time": "09:41" }
//   ]}
// Empty array = no notifications.
#define API_HOST      "192.168.1.100"   // Clawpilot PC IP
#define API_PORT      8222
#define API_PATH      "/api/notifications"
#define POLL_INTERVAL 5000    // ms between polls

// ────────────────────────────────────────────
//  NTP Time Sync
// ────────────────────────────────────────────
#define NTP_SERVER    "pool.ntp.org"
#define GMT_OFFSET    3600    // CET = UTC+1 (seconds)
#define DST_OFFSET    3600    // CEST daylight saving (seconds)

// ────────────────────────────────────────────
//  HARDWARE PLATFORM SELECTION
// ────────────────────────────────────────────
// Uncomment ONE of these:
//   PLATFORM_WAVESHARE  — Real hardware (ESP32-C6 + ST7789 172×320)
//   PLATFORM_WOKWI      — Wokwi simulator (ESP32 DevKit + ILI9341 240×320)

// #define PLATFORM_WAVESHARE
#define PLATFORM_WOKWI

// ────────────────────────────────────────────
//  WAVESHARE ESP32-C6 LCD 1.47" Pin Config
// ────────────────────────────────────────────
#ifdef PLATFORM_WAVESHARE
  #define TFT_MOSI    6
  #define TFT_SCLK    7
  #define TFT_MISO    5
  #define TFT_CS      14
  #define TFT_DC      15
  #define TFT_RST     21
  #define TFT_BL      22      // backlight PWM
  #define SW          172
  #define SH          320
#endif

// ────────────────────────────────────────────
//  WOKWI EMULATOR Pin Config (ESP32 DevKit C V4)
// ────────────────────────────────────────────
#ifdef PLATFORM_WOKWI
  #define TFT_MOSI    23
  #define TFT_SCLK    18
  #define TFT_MISO    19
  #define TFT_CS      15
  #define TFT_DC      2
  #define TFT_RST     -1      // no RST on Wokwi ILI9341
  #define TFT_BL      -1      // no backlight control
  #define SW          240
  #define SH          320
#endif


// ── SKETCH HEAD (includes + tft) ──
/*
 * ╔═════════════════════════════════════════╗
 * ║  RED DEVICE — ClawMagotchi v0.2         ║
 * ║  Clock + Lobster + WiFi + Notifications ║
 * ║  ST7789 (real) / ILI9341 (Wokwi)       ║
 * ╚═════════════════════════════════════════╝
 *
 * Idle screen: a digital clock with an animated
 * lobster companion roaming the bottom of the
 * display. Connects to WiFi, syncs time via NTP,
 * and polls Clawpilot for live notifications.
 *
 * Hardware:  Waveshare ESP32-C6 LCD 1.47" (ST7789)
 * Emulator:  Wokwi with ILI9341 (240×320)
 */

#include <SPI.h>
#include <Adafruit_GFX.h>

#ifdef PLATFORM_WAVESHARE
  #include <Adafruit_ST7789.h>
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <ArduinoJson.h>
  #include <time.h>
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#else
  #include <Adafruit_ILI9341.h>
  Adafruit_ILI9341 tft(TFT_CS, TFT_DC);


// ═══════════════════════════════════════════
// FORWARD TYPE DECLARATIONS
// (Arduino IDE generates function prototypes before these,
//  so they must appear before any function definitions)
// ═══════════════════════════════════════════
#define _LMOOD_DEFINED
enum LMood { IDLE, HAPPY, ALERT, URGENT };

enum LobsterExpr { EXPR_NORMAL, EXPR_HAPPY, EXPR_ALERT, EXPR_SLEEPY, EXPR_DIZZY };
enum ClawPose { CLAW_IDLE, CLAW_RAISED, CLAW_HOLDING, CLAW_TUCKED };

struct LobsterPose {
  int cx, cy;
  int frame;
  int dir;
  LobsterExpr expr;
  ClawPose clawPose;
  const uint8_t* heldIcon;
  uint16_t heldColor;
  float bobOffset;
  float swayOffset;
};

typedef void (*IconDrawFn)(int cx, int cy);

struct ScenarioV2Config {
  const char* message;
  IconDrawFn drawIcon;
  uint16_t accentColor;
  LobsterExpr expression;
  ClawPose clawPose;
  const uint8_t* heldItem;
  uint16_t heldColor;
};

enum ScenarioV2 {
  S2_IDLE=0,S2_FOCUS,S2_CONTEXT_SWITCH,S2_TASK_NUDGE,S2_LONG_SESSION,
  S2_MEETING_SOON,S2_LATE,S2_EMAIL,S2_TEAMS_MSG,S2_WORTH_REPLY,
  S2_QUIET_TOO_LONG,S2_MOVE,S2_HYDRATE,S2_MORNING,S2_SHUTDOWN,
  S2_WORKING,S2_DONE,S2_NEEDS_INPUT,S2_STUCK,S2_DECISION,
  S2_DOOMSCROLL,S2_CELEBRATION,S2_IDLE_PERSONALITY,S2_COUNT
};

enum Scenario {
  SC_IDLE=0,SC_FOCUS,SC_CONTEXT_SWITCH,SC_TASK_NUDGE,SC_LONG_SESSION,
  SC_MEETING_SOON,SC_LATE,SC_EMAIL,SC_TEAMS_MSG,SC_WORTH_REPLY,
  SC_QUIET_TOO_LONG,SC_MOVE,SC_HYDRATE,SC_MORNING,SC_SHUTDOWN,
  SC_WORKING,SC_DONE,SC_NEEDS_INPUT,SC_STUCK,SC_DECISION,
  SC_DOOMSCROLL,SC_CELEBRATION,SC_IDLE_PERSONALITY,SC_COUNT
};

struct ScenarioConfig {
  const char* message;
  const uint8_t* icon;
  uint16_t iconColor;
  uint16_t accentColor;
  LMood lobsterMood;
  bool hasHeldItem;
  const uint8_t* heldIcon;
};

// ── SEGMENTS.H ──
/*
 * ╔═════════════════════════════════════════╗
 * ║  segments.h — 7-Segment Clock Renderer  ║
 * ║  Procedural rendering via fillRect()    ║
 * ╚═════════════════════════════════════════╝
 */

// ────────────────────────────────────────────
//  Colors
// ────────────────────────────────────────────
#define SEG_ON   0x07F1   // bright green #00FF88
#define SEG_OFF  0x0220   // ghost green (very dim)
#define SEG_BG   0x18C5   // dark background (= C_BG)

// ────────────────────────────────────────────
//  Digit sizing — adapts to screen width
// ────────────────────────────────────────────
#if SW >= 200
  #define SEG_DW    30    // digit width
  #define SEG_DH    50    // digit height
  #define SEG_TH    5     // segment thickness
  #define SEG_GAP   6     // gap between digits
  #define SEG_Y0    40    // Y origin for clock
  #define SEG_SEC_S 2     // seconds text size
#else
  #define SEG_DW    22    // narrower for 172px
  #define SEG_DH    38
  #define SEG_TH    4
  #define SEG_GAP   4
  #define SEG_Y0    44
  #define SEG_SEC_S 1
#endif

// Colon dimensions
#define SEG_COL_R   3     // colon dot radius
#define SEG_COL_GAP 10    // spacing between pair

// ────────────────────────────────────────────
//  Segment map: 7 segments labeled a-g
//
//   ─── a ───
//  |         |
//  f         b
//  |         |
//   ─── g ───
//  |         |
//  e         c
//  |         |
//   ─── d ───
//
//  Bit order: 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g
// ────────────────────────────────────────────
static const uint8_t SEG_MAP[10] PROGMEM = {
  0b0111111, // 0: a b c d e f
  0b0000110, // 1: b c
  0b1011011, // 2: a b d e g
  0b1001111, // 3: a b c d g
  0b1100110, // 4: b c f g
  0b1101101, // 5: a c d f g
  0b1111101, // 6: a c d e f g
  0b0000111, // 7: a b c
  0b1111111, // 8: all
  0b1101111, // 9: a b c d f g
};

// ────────────────────────────────────────────
//  Internal: draw a single segment
//  seg 0-6 = a-g, at digit origin (dx, dy)
// ────────────────────────────────────────────
inline void _seg_draw(int dx, int dy, int seg, uint16_t color) {
  int w = SEG_DW;
  int h = SEG_DH;
  int t = SEG_TH;
  int m = 2; // margin to avoid overlap at corners

  switch (seg) {
    case 0: // a — top horizontal
      tft.fillRoundRect(dx + m, dy, w - 2*m, t, 1, color);
      break;
    case 1: // b — top-right vertical
      tft.fillRoundRect(dx + w - t, dy + m, t, h/2 - m, 1, color);
      break;
    case 2: // c — bottom-right vertical
      tft.fillRoundRect(dx + w - t, dy + h/2 + m, t, h/2 - m, 1, color);
      break;
    case 3: // d — bottom horizontal
      tft.fillRoundRect(dx + m, dy + h - t, w - 2*m, t, 1, color);
      break;
    case 4: // e — bottom-left vertical
      tft.fillRoundRect(dx, dy + h/2 + m, t, h/2 - m, 1, color);
      break;
    case 5: // f — top-left vertical
      tft.fillRoundRect(dx, dy + m, t, h/2 - m, 1, color);
      break;
    case 6: // g — middle horizontal
      tft.fillRoundRect(dx + m, dy + h/2 - t/2, w - 2*m, t, 1, color);
      break;
  }
}

// ────────────────────────────────────────────
//  Draw a single digit 0-9 at position (dx, dy)
// ────────────────────────────────────────────
inline void _seg_digit(int dx, int dy, int digit) {
  uint8_t bits = pgm_read_byte(&SEG_MAP[digit]);
  for (int s = 0; s < 7; s++) {
    uint16_t color = (bits & (1 << s)) ? SEG_ON : SEG_OFF;
    _seg_draw(dx, dy, s, color);
  }
}

// ────────────────────────────────────────────
//  Public API
// ────────────────────────────────────────────

inline void seg7_init() {
  // Nothing required — stateless renderer
}

/*
 * Render HH:MM in 7-segment style, centered on screen.
 * colonOn: true = bright colon, false = dim colon
 */
inline void seg7_renderTime(int h, int m, bool colonOn) {
  // Total width: 4 digits + 3 gaps + colon space
  int colonW = SEG_COL_R * 2 + 4;
  int totalW = 4 * SEG_DW + 2 * SEG_GAP + colonW;
  int x0 = (SW - totalW) / 2;
  int y = SEG_Y0;

  // Clear clock area
  tft.fillRect(x0 - 2, y - 2, totalW + 4, SEG_DH + 4, SEG_BG);

  int x = x0;

  // Hour tens
  _seg_digit(x, y, h / 10);
  x += SEG_DW + SEG_GAP;

  // Hour ones
  _seg_digit(x, y, h % 10);
  x += SEG_DW + SEG_GAP / 2;

  // Colon
  int colonX = x + colonW / 2;
  uint16_t cCol = colonOn ? SEG_ON : SEG_OFF;
  tft.fillCircle(colonX, y + SEG_DH / 3, SEG_COL_R, cCol);
  tft.fillCircle(colonX, y + 2 * SEG_DH / 3, SEG_COL_R, cCol);
  x += colonW + SEG_GAP / 2;

  // Minute tens
  _seg_digit(x, y, m / 10);
  x += SEG_DW + SEG_GAP;

  // Minute ones
  _seg_digit(x, y, m % 10);
}

/*
 * Render seconds smaller below the main clock
 */
inline void seg7_renderSec(int s) {
  int secY = SEG_Y0 + SEG_DH + 8;
  int secW = 24;  // approx width of "SS" text at size 2
  int x = (SW - secW) / 2;

  tft.fillRect(x - 2, secY - 1, secW + 4, 18, SEG_BG);
  tft.setTextSize(SEG_SEC_S + 1);
  tft.setTextColor(SEG_ON, SEG_BG);
  tft.setCursor(x, secY);
  if (s < 10) tft.print('0');
  tft.print(s);
}

/*
 * Render a date string centered below seconds
 */
inline void seg7_renderDate(const char* dateStr) {
  int dateY = SEG_Y0 + SEG_DH + 30;
  int w = strlen(dateStr) * 6; // size 1 char = 6px wide
  int x = (SW - w) / 2;

  tft.fillRect(0, dateY - 1, SW, 12, SEG_BG);
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF, SEG_BG); // C_GRAY
  tft.setCursor(x, dateY);
  tft.print(dateStr);
}


// ── SPRITES.H (16x16 icons for held items) ──
/*
 * ╔═════════════════════════════════════════╗
 * ║  sprites.h — 16×16 Icon Sprite System   ║
 * ║  1-bit PROGMEM bitmaps + draw helpers   ║
 * ╚═════════════════════════════════════════╝
 */

// ────────────────────────────────────────────
//  16×16 Icon Bitmaps (1-bit, 32 bytes each)
//  Stored MSB-first, row by row (Adafruit GFX format)
// ────────────────────────────────────────────

// ✉️ Envelope — rectangle with V-flap
static const uint8_t ICON_ENVELOPE[] PROGMEM = {
  0x00,0x00, 0x00,0x00, 0x7F,0xFE, 0x7F,0xFE,
  0x70,0x0E, 0x78,0x1E, 0x6C,0x36, 0x66,0x66,
  0x63,0xC6, 0x61,0x86, 0x60,0x06, 0x60,0x06,
  0x60,0x06, 0x7F,0xFE, 0x00,0x00, 0x00,0x00,
};

// 💬 Chat bubble — rounded rect with tail
static const uint8_t ICON_CHAT[] PROGMEM = {
  0x00,0x00, 0x0F,0xF0, 0x1F,0xF8, 0x3F,0xFC,
  0x30,0x0C, 0x30,0x0C, 0x30,0x0C, 0x30,0x0C,
  0x3F,0xFC, 0x1F,0xF8, 0x0F,0xF0, 0x06,0x00,
  0x07,0x00, 0x03,0x80, 0x01,0x00, 0x00,0x00,
};

// 🕒 Clock — circle with hands
static const uint8_t ICON_CLOCK[] PROGMEM = {
  0x00,0x00, 0x07,0xE0, 0x1F,0xF8, 0x38,0x1C,
  0x30,0x0C, 0x61,0x86, 0x61,0x86, 0x61,0x86,
  0x61,0xFE, 0x61,0xFE, 0x60,0x06, 0x30,0x0C,
  0x38,0x1C, 0x1F,0xF8, 0x07,0xE0, 0x00,0x00,
};

// ⚠️ Warning triangle with exclamation
static const uint8_t ICON_WARNING[] PROGMEM = {
  0x00,0x00, 0x01,0x80, 0x01,0x80, 0x03,0xC0,
  0x03,0xC0, 0x06,0x60, 0x06,0x60, 0x0C,0x30,
  0x0D,0xB0, 0x19,0x98, 0x19,0x98, 0x30,0x0C,
  0x31,0x8C, 0x60,0x06, 0x7F,0xFE, 0x00,0x00,
};

// ✅ Checkmark
static const uint8_t ICON_CHECK[] PROGMEM = {
  0x00,0x00, 0x00,0x00, 0x00,0x06, 0x00,0x0E,
  0x00,0x1C, 0x00,0x38, 0x00,0x70, 0x00,0xE0,
  0x61,0xC0, 0x73,0x80, 0x3F,0x00, 0x1E,0x00,
  0x0C,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

// ❓ Question mark
static const uint8_t ICON_QUESTION[] PROGMEM = {
  0x00,0x00, 0x07,0xE0, 0x0F,0xF0, 0x1C,0x38,
  0x18,0x18, 0x00,0x38, 0x00,0x70, 0x00,0xE0,
  0x01,0xC0, 0x01,0x80, 0x01,0x80, 0x00,0x00,
  0x01,0x80, 0x01,0x80, 0x00,0x00, 0x00,0x00,
};

// 💧 Droplet — teardrop shape
static const uint8_t ICON_DROPLET[] PROGMEM = {
  0x00,0x00, 0x01,0x80, 0x01,0x80, 0x03,0xC0,
  0x03,0xC0, 0x07,0xE0, 0x07,0xE0, 0x0F,0xF0,
  0x1F,0xF8, 0x1F,0xF8, 0x1F,0xF8, 0x1F,0xF8,
  0x0F,0xF0, 0x07,0xE0, 0x03,0xC0, 0x00,0x00,
};

// 🏃 Runner — stick figure
static const uint8_t ICON_RUNNER[] PROGMEM = {
  0x00,0x00, 0x03,0x80, 0x03,0x80, 0x03,0x80,
  0x01,0x00, 0x07,0xE0, 0x0F,0xF0, 0x19,0x00,
  0x01,0x00, 0x01,0x00, 0x03,0x80, 0x06,0xC0,
  0x0C,0x60, 0x18,0x30, 0x10,0x10, 0x00,0x00,
};

// 🛡️ Shield
static const uint8_t ICON_SHIELD[] PROGMEM = {
  0x00,0x00, 0x1F,0xF8, 0x3F,0xFC, 0x7F,0xFE,
  0x7F,0xFE, 0x7F,0xFE, 0x7F,0xFE, 0x7F,0xFE,
  0x7F,0xFE, 0x3F,0xFC, 0x3F,0xFC, 0x1F,0xF8,
  0x0F,0xF0, 0x07,0xE0, 0x03,0xC0, 0x01,0x80,
};

// 🌅 Sunrise — half circle with rays
static const uint8_t ICON_SUNRISE[] PROGMEM = {
  0x01,0x80, 0x01,0x80, 0x08,0x10, 0x04,0x20,
  0x01,0x80, 0x21,0x84, 0x10,0x08, 0x00,0x00,
  0x07,0xE0, 0x1F,0xF8, 0x3F,0xFC, 0x3F,0xFC,
  0x7F,0xFE, 0xFF,0xFF, 0xFF,0xFF, 0x00,0x00,
};

// 🌙 Moon — crescent
static const uint8_t ICON_MOON[] PROGMEM = {
  0x00,0x00, 0x03,0xE0, 0x0F,0xC0, 0x1F,0x80,
  0x3F,0x00, 0x3E,0x00, 0x7E,0x00, 0x7E,0x00,
  0x7E,0x00, 0x7E,0x00, 0x3E,0x00, 0x3F,0x00,
  0x1F,0x80, 0x0F,0xC0, 0x03,0xE0, 0x00,0x00,
};

// ⚙️ Gear — circle with teeth
static const uint8_t ICON_GEAR[] PROGMEM = {
  0x03,0xC0, 0x03,0xC0, 0x0F,0xF0, 0x0F,0xF0,
  0x3F,0xFC, 0x3F,0xFC, 0xFC,0x3F, 0xF8,0x1F,
  0xF8,0x1F, 0xFC,0x3F, 0x3F,0xFC, 0x3F,0xFC,
  0x0F,0xF0, 0x0F,0xF0, 0x03,0xC0, 0x03,0xC0,
};

// ⚖️ Scales — balance
static const uint8_t ICON_SCALES[] PROGMEM = {
  0x01,0x80, 0x01,0x80, 0x7F,0xFE, 0x7F,0xFE,
  0x21,0x84, 0x11,0x88, 0x09,0x90, 0x09,0x90,
  0x0D,0xB0, 0x0D,0xB0, 0x1C,0x38, 0x1F,0xF8,
  0x00,0x00, 0x01,0x80, 0x01,0x80, 0x07,0xE0,
};

// 🎉 Star — 5-pointed
static const uint8_t ICON_STAR[] PROGMEM = {
  0x01,0x80, 0x01,0x80, 0x03,0xC0, 0x03,0xC0,
  0xFF,0xFF, 0x7F,0xFE, 0x3F,0xFC, 0x0F,0xF0,
  0x0F,0xF0, 0x1F,0xF8, 0x1C,0x38, 0x38,0x1C,
  0x30,0x0C, 0x60,0x06, 0x40,0x02, 0x00,0x00,
};

// ↩️ Reply arrow — curved arrow pointing left
static const uint8_t ICON_REPLY[] PROGMEM = {
  0x00,0x00, 0x00,0x00, 0x04,0x00, 0x0C,0x00,
  0x1F,0xFC, 0x3F,0xFC, 0x7F,0xFC, 0x3C,0x0C,
  0x1C,0x0C, 0x0C,0x0C, 0x04,0x1C, 0x00,0x38,
  0x00,0x70, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

// 👀 Eye
static const uint8_t ICON_EYE[] PROGMEM = {
  0x00,0x00, 0x00,0x00, 0x00,0x00, 0x07,0xE0,
  0x1F,0xF8, 0x3F,0xFC, 0x78,0x1E, 0x73,0xCE,
  0x73,0xCE, 0x78,0x1E, 0x3F,0xFC, 0x1F,0xF8,
  0x07,0xE0, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

// ────────────────────────────────────────────
//  Drawing Functions
// ────────────────────────────────────────────

/*
 * Draw a 16×16 1-bit icon at (x, y) with given color.
 * Transparent background (only draws ON pixels).
 */
inline void drawIcon16(int x, int y, const uint8_t* icon, uint16_t color) {
  for (int row = 0; row < 16; row++) {
    uint8_t b0 = pgm_read_byte(&icon[row * 2]);
    uint8_t b1 = pgm_read_byte(&icon[row * 2 + 1]);
    uint16_t rowBits = ((uint16_t)b0 << 8) | b1;
    for (int col = 0; col < 16; col++) {
      if (rowBits & (0x8000 >> col)) {
        tft.drawPixel(x + col, y + row, color);
      }
    }
  }
}

/*
 * Draw icon with a circular background badge (notification style).
 * Draws a filled circle of bgColor, then the icon on top.
 */
inline void drawIconBadge(int x, int y, const uint8_t* icon, uint16_t iconColor, uint16_t bgColor) {
  // Badge circle: centered on the 16×16 icon area
  int cx = x + 8;
  int cy = y + 8;
  tft.fillCircle(cx, cy, 10, bgColor);
  drawIcon16(x, y, icon, iconColor);
}


// ── ICONS_LARGE.H (procedural large icons) ──
/*
 * ╔═════════════════════════════════════════╗
 * ║  icons_large.h — Large Notification     ║
 * ║  Icons (32-60px procedural drawing)     ║
 * ║  Matches Designer.png reference style   ║
 * ╚═════════════════════════════════════════╝
 *
 * These are drawn procedurally — zero bitmap memory.
 * Each icon is ~40-50px and uses fillCircle, fillTriangle
 * etc. for a polished, multi-color look.
 */

// Icon color definitions
#define IC_RED      0xE1C8
#define IC_DRED     0xC083
#define IC_GOLD     0xFE8C
#define IC_WARN     0xFD20   // orange
#define IC_BLUE     0x2C9F
#define IC_DBLUE    0x1A5F
#define IC_CYAN     0x07FF
#define IC_GREEN    0x07E0
#define IC_DGREEN   0x03E0
#define IC_PURPLE   0x541F
#define IC_WHITE    0xFFFF
#define IC_BLACK    0x0000
#define IC_GRAY     0x7BEF
#define IC_LGRAY    0xAD75
#define IC_YELLOW   0xFFE0

// ────────────────────────────────────────────
//  Warning Triangle (⚠️) — ~48px
//  Red triangle with black "!" and yellow sparks
// ────────────────────────────────────────────
inline void drawIconLargeWarning(int cx, int cy) {
  // Main triangle
  int h = 40;
  int w = 46;
  tft.fillTriangle(cx, cy - h/2, cx - w/2, cy + h/2, cx + w/2, cy + h/2, IC_RED);
  // Inner slightly smaller triangle for border effect
  tft.fillTriangle(cx, cy - h/2 + 4, cx - w/2 + 5, cy + h/2 - 2, cx + w/2 - 5, cy + h/2 - 2, IC_DRED);
  // Black exclamation mark
  tft.fillRect(cx - 2, cy - 10, 5, 16, IC_BLACK);
  tft.fillCircle(cx, cy + 12, 3, IC_BLACK);
  // Yellow spark lines on sides (like Designer.png)
  tft.drawLine(cx - w/2 - 6, cy - 2, cx - w/2 - 14, cy - 6, IC_YELLOW);
  tft.drawLine(cx - w/2 - 4, cy + 4, cx - w/2 - 12, cy + 6, IC_YELLOW);
  tft.drawLine(cx + w/2 + 6, cy - 2, cx + w/2 + 14, cy - 6, IC_YELLOW);
  tft.drawLine(cx + w/2 + 4, cy + 4, cx + w/2 + 12, cy + 6, IC_YELLOW);
}

// ────────────────────────────────────────────
//  Envelope (✉️) — ~44px
//  Blue rectangle with white V-flap
// ────────────────────────────────────────────
inline void drawIconLargeEnvelope(int cx, int cy) {
  int w = 44, h = 30;
  int x0 = cx - w/2, y0 = cy - h/2;
  // Main body
  tft.fillRoundRect(x0, y0, w, h, 3, IC_BLUE);
  // Darker bottom
  tft.fillRect(x0 + 2, cy, w - 4, h/2 - 2, IC_DBLUE);
  // V-flap (white triangle on top)
  tft.fillTriangle(x0 + 2, y0 + 2, cx, cy + 2, x0 + w - 2, y0 + 2, IC_WHITE);
  // Outline the V
  tft.drawLine(x0 + 2, y0 + 2, cx, cy + 2, IC_DBLUE);
  tft.drawLine(cx, cy + 2, x0 + w - 2, y0 + 2, IC_DBLUE);
}

// ────────────────────────────────────────────
//  Chat Bubble (💬) — ~44px
//  Purple rounded rect with tail and dots
// ────────────────────────────────────────────
inline void drawIconLargeChat(int cx, int cy) {
  int w = 44, h = 28;
  int x0 = cx - w/2, y0 = cy - h/2 - 4;
  // Main bubble
  tft.fillRoundRect(x0, y0, w, h, 6, IC_PURPLE);
  // Tail triangle
  tft.fillTriangle(cx - 8, y0 + h - 1, cx - 2, y0 + h - 1, cx - 12, y0 + h + 10, IC_PURPLE);
  // Three dots (typing indicator)
  int dotY = y0 + h/2;
  tft.fillCircle(cx - 10, dotY, 3, IC_WHITE);
  tft.fillCircle(cx,      dotY, 3, IC_WHITE);
  tft.fillCircle(cx + 10, dotY, 3, IC_WHITE);
}

// ────────────────────────────────────────────
//  Clock (🕒) — ~44px
//  Circle with hour/minute hands
// ────────────────────────────────────────────
inline void drawIconLargeClock(int cx, int cy) {
  int r = 22;
  // Outer ring
  tft.fillCircle(cx, cy, r, IC_BLUE);
  tft.fillCircle(cx, cy, r - 3, IC_DBLUE);
  // Face
  tft.fillCircle(cx, cy, r - 4, IC_WHITE);
  // Hour marks
  for (int i = 0; i < 12; i++) {
    float angle = i * 30.0 * 3.14159 / 180.0;
    int mx = cx + (int)((r - 7) * sin(angle));
    int my = cy - (int)((r - 7) * cos(angle));
    tft.fillCircle(mx, my, 1, IC_GRAY);
  }
  // Hour hand (pointing to ~10)
  tft.drawLine(cx, cy, cx - 8, cy - 10, IC_BLACK);
  tft.drawLine(cx + 1, cy, cx - 7, cy - 10, IC_BLACK);
  // Minute hand (pointing to ~2)
  tft.drawLine(cx, cy, cx + 10, cy - 12, IC_DBLUE);
  // Center dot
  tft.fillCircle(cx, cy, 2, IC_RED);
}

// ────────────────────────────────────────────
//  Checkmark (✅) — ~40px
//  Green circle with white check
// ────────────────────────────────────────────
inline void drawIconLargeCheck(int cx, int cy) {
  tft.fillCircle(cx, cy, 20, IC_GREEN);
  tft.fillCircle(cx, cy, 17, IC_DGREEN);
  // Thick white checkmark
  for (int t = -1; t <= 1; t++) {
    tft.drawLine(cx - 10, cy + t, cx - 3, cy + 8 + t, IC_WHITE);
    tft.drawLine(cx - 3, cy + 8 + t, cx + 10, cy - 6 + t, IC_WHITE);
  }
}

// ────────────────────────────────────────────
//  Question mark (❓) — ~40px
//  Gold circle with dark "?"
// ────────────────────────────────────────────
inline void drawIconLargeQuestion(int cx, int cy) {
  tft.fillCircle(cx, cy, 20, IC_GOLD);
  // "?" drawn with primitives
  tft.drawCircleHelper(cx + 2, cy - 6, 8, 0x03, IC_BLACK);  // top curve
  tft.drawCircleHelper(cx + 2, cy - 6, 7, 0x03, IC_BLACK);
  tft.fillRect(cx - 1, cy - 2, 4, 8, IC_BLACK);              // stem
  tft.fillCircle(cx, cy + 10, 2, IC_BLACK);                   // dot
}

// ────────────────────────────────────────────
//  Shield (🛡️) — ~40px (focus mode)
// ────────────────────────────────────────────
inline void drawIconLargeShield(int cx, int cy) {
  // Shield shape: wide top, pointed bottom
  tft.fillRoundRect(cx - 16, cy - 18, 32, 24, 4, IC_GREEN);
  tft.fillTriangle(cx - 16, cy + 4, cx + 16, cy + 4, cx, cy + 20, IC_GREEN);
  // Inner
  tft.fillRoundRect(cx - 12, cy - 14, 24, 18, 3, IC_DGREEN);
  tft.fillTriangle(cx - 12, cy + 2, cx + 12, cy + 2, cx, cy + 16, IC_DGREEN);
  // Check inside shield
  for (int t = 0; t <= 1; t++) {
    tft.drawLine(cx - 6, cy - 2 + t, cx - 2, cy + 4 + t, IC_WHITE);
    tft.drawLine(cx - 2, cy + 4 + t, cx + 6, cy - 6 + t, IC_WHITE);
  }
}

// ────────────────────────────────────────────
//  Droplet (💧) — ~40px
// ────────────────────────────────────────────
inline void drawIconLargeDroplet(int cx, int cy) {
  // Teardrop: triangle top + circle bottom
  tft.fillTriangle(cx, cy - 18, cx - 12, cy, cx + 12, cy, IC_CYAN);
  tft.fillCircle(cx, cy + 2, 12, IC_CYAN);
  // Highlight
  tft.fillCircle(cx - 3, cy - 4, 3, IC_WHITE);
}

// ────────────────────────────────────────────
//  Sunrise (🌅) — ~44px
// ────────────────────────────────────────────
inline void drawIconLargeSunrise(int cx, int cy) {
  // Sun half-circle
  tft.fillCircle(cx, cy + 6, 14, IC_GOLD);
  // Horizon line covers bottom
  tft.fillRect(cx - 24, cy + 6, 48, 16, IC_WARN);
  // Rays
  for (int i = 0; i < 5; i++) {
    float angle = (30 + i * 30) * 3.14159 / 180.0;
    int rx = cx + (int)(22 * cos(angle));
    int ry = cy + 4 - (int)(22 * sin(angle));
    tft.drawLine(cx, cy + 4, rx, ry, IC_YELLOW);
  }
}

// ────────────────────────────────────────────
//  Moon (🌙) — ~40px
// ────────────────────────────────────────────
inline void drawIconLargeMoon(int cx, int cy) {
  tft.fillCircle(cx, cy, 16, IC_GOLD);
  tft.fillCircle(cx + 8, cy - 4, 14, IC_BLACK);  // shadow cutout — will need BG color
}

// ────────────────────────────────────────────
//  Gear (⚙️) — ~40px
// ────────────────────────────────────────────
inline void drawIconLargeGear(int cx, int cy) {
  int r = 16;
  tft.fillCircle(cx, cy, r, IC_GRAY);
  // Teeth (8 rectangles around edge)
  for (int i = 0; i < 8; i++) {
    float angle = i * 45.0 * 3.14159 / 180.0;
    int tx = cx + (int)(r * sin(angle));
    int ty = cy - (int)(r * cos(angle));
    tft.fillRect(tx - 3, ty - 3, 6, 6, IC_GRAY);
  }
  // Inner ring
  tft.fillCircle(cx, cy, r - 5, IC_LGRAY);
  // Center hole
  tft.fillCircle(cx, cy, 4, IC_BLACK);
}

// ────────────────────────────────────────────
//  Star (🎉) — ~40px celebration
// ────────────────────────────────────────────
inline void drawIconLargeStar(int cx, int cy) {
  // Simple 5-pointed star using triangles
  tft.fillTriangle(cx, cy - 18, cx - 6, cy - 4, cx + 6, cy - 4, IC_GOLD);
  tft.fillTriangle(cx - 18, cy - 4, cx - 4, cy - 4, cx - 8, cy + 8, IC_GOLD);
  tft.fillTriangle(cx + 18, cy - 4, cx + 4, cy - 4, cx + 8, cy + 8, IC_GOLD);
  tft.fillTriangle(cx - 12, cy + 16, cx, cy + 4, cx - 14, cy + 2, IC_GOLD);
  tft.fillTriangle(cx + 12, cy + 16, cx, cy + 4, cx + 14, cy + 2, IC_GOLD);
  // Center fill
  tft.fillCircle(cx, cy, 6, IC_GOLD);
}

// ────────────────────────────────────────────
//  Runner (🏃) — ~40px
// ────────────────────────────────────────────
inline void drawIconLargeRunner(int cx, int cy) {
  // Head
  tft.fillCircle(cx + 4, cy - 14, 5, IC_WARN);
  // Torso
  tft.drawLine(cx, cy - 8, cx - 2, cy + 4, IC_WARN);
  tft.drawLine(cx + 1, cy - 8, cx - 1, cy + 4, IC_WARN);
  // Arms
  tft.drawLine(cx - 1, cy - 5, cx + 10, cy - 10, IC_WARN);
  tft.drawLine(cx - 1, cy - 5, cx - 10, cy, IC_WARN);
  // Legs (running pose)
  tft.drawLine(cx - 2, cy + 4, cx - 10, cy + 16, IC_WARN);
  tft.drawLine(cx - 2, cy + 4, cx + 8, cy + 16, IC_WARN);
}

// ────────────────────────────────────────────
//  Eye (👀) — for doomscroll / quiet
// ────────────────────────────────────────────
inline void drawIconLargeEye(int cx, int cy) {
  // Outer eye shape (almond)
  tft.fillCircle(cx, cy, 14, IC_WHITE);
  tft.fillCircle(cx - 16, cy, 6, IC_WHITE);
  tft.fillCircle(cx + 16, cy, 6, IC_WHITE);
  // Iris
  tft.fillCircle(cx, cy, 8, IC_CYAN);
  // Pupil
  tft.fillCircle(cx, cy, 4, IC_BLACK);
  // Highlight
  tft.fillCircle(cx - 2, cy - 3, 2, IC_WHITE);
}

// ────────────────────────────────────────────
//  Scales (⚖️) — decision
// ────────────────────────────────────────────
inline void drawIconLargeScales(int cx, int cy) {
  // Vertical post
  tft.fillRect(cx - 1, cy - 16, 3, 32, IC_GOLD);
  // Beam
  tft.fillRect(cx - 20, cy - 14, 40, 3, IC_GOLD);
  // Left pan
  tft.drawLine(cx - 20, cy - 14, cx - 16, cy + 2, IC_GOLD);
  tft.drawLine(cx - 20, cy - 14, cx - 24, cy + 2, IC_GOLD);
  tft.fillRect(cx - 26, cy + 2, 14, 3, IC_GOLD);
  // Right pan
  tft.drawLine(cx + 20, cy - 14, cx + 16, cy + 2, IC_GOLD);
  tft.drawLine(cx + 20, cy - 14, cx + 24, cy + 2, IC_GOLD);
  tft.fillRect(cx + 14, cy + 2, 14, 3, IC_GOLD);
  // Base
  tft.fillRect(cx - 8, cy + 14, 16, 3, IC_GOLD);
}

// ────────────────────────────────────────────
//  Reply Arrow (↩️)
// ────────────────────────────────────────────
inline void drawIconLargeReply(int cx, int cy) {
  // Curved arrow body
  tft.drawCircleHelper(cx + 4, cy - 2, 14, 0x06, IC_BLUE);
  tft.drawCircleHelper(cx + 4, cy - 2, 13, 0x06, IC_BLUE);
  tft.drawCircleHelper(cx + 4, cy - 2, 12, 0x06, IC_BLUE);
  // Arrow head
  tft.fillTriangle(cx - 14, cy - 2, cx - 6, cy - 10, cx - 6, cy + 6, IC_BLUE);
}


// ── LOBSTER_GFX.H (composited lobster renderer) ──
/*
 * ╔═════════════════════════════════════════╗
 * ║  lobster_gfx.h — Composited Lobster    ║
 * ║  Large procedural lobster matching      ║
 * ║  the Designer.png reference art         ║
 * ╚═════════════════════════════════════════╝
 *
 * Architecture: Body + Eyes + Claws + Antennae + Legs
 * Each part drawn with GFX primitives (fillCircle, fillTriangle etc.)
 * Body: ~80×60px, centered at (cx, cy)
 * Eyes: 2× cyan circles with white highlight
 * Claws: gold pincers, animated open/close
 * Held items: overlaid on claw when scenario demands it
 *
 * Memory: 0 bytes of sprite data — 100% procedural
 */

// ────────────────────────────────────────────
//  Lobster Color Palette (RGB565)
// ────────────────────────────────────────────
#define LB_RED     0xE1C8   // main body red #E63946
#define LB_DRED    0xC083   // shadow/dark red #C1121F
#define LB_LRED    0xF2CA   // highlight red (lighter)
#define LB_GOLD    0xFE8C   // claw gold #FFD166
#define LB_DGOLD   0xD5A4   // dark gold for claw tips
#define LB_CYAN    0x07FF   // eye cyan #00FFFF
#define LB_DCYAN   0x0677   // darker cyan for eye ring
#define LB_WHITE   0xFFFF
#define LB_BLACK   0x0000
#define LB_BG      0x18C5   // background color

// ────────────────────────────────────────────
//  Expression enum — composited onto face
// ────────────────────────────────────────────


// ────────────────────────────────────────────
//  Claw pose enum
// ────────────────────────────────────────────


// ────────────────────────────────────────────
//  Lobster render state (passed to drawLobsterV2)
// ────────────────────────────────────────────


// Forward declare the 16×16 icon drawer from sprites.h


// ────────────────────────────────────────────
//  MAIN DRAW FUNCTION
//  Draws the full lobster at given pose.
//  ~80px wide, ~65px tall from antenna tip to feet.
// ────────────────────────────────────────────

inline void drawLobsterV2(const LobsterPose& p) {
  int cx = p.cx + (int)p.swayOffset;
  int cy = p.cy + (int)p.bobOffset;
  int d  = p.dir;
  int f  = p.frame;

  // ═══════════════════════════════════════
  //  BODY — large oval (main torso)
  // ═══════════════════════════════════════
  // Main body: wide ellipse approximated with overlapping circles
  tft.fillCircle(cx, cy, 20, LB_RED);           // center body
  tft.fillCircle(cx - 6, cy, 18, LB_RED);       // left extension
  tft.fillCircle(cx + 6, cy, 18, LB_RED);       // right extension
  // Belly highlight (lighter arc at top)
  tft.fillCircle(cx, cy - 4, 14, LB_LRED);
  // Re-cover bottom with main red for shadow effect
  tft.fillRect(cx - 18, cy + 2, 36, 16, LB_RED);
  tft.fillCircle(cx - 10, cy + 12, 8, LB_RED);
  tft.fillCircle(cx + 10, cy + 12, 8, LB_RED);
  // Bottom shadow
  tft.fillCircle(cx, cy + 14, 12, LB_DRED);

  // ═══════════════════════════════════════
  //  EYES — the signature cyan glow
  // ═══════════════════════════════════════
  int eyeSpacing = 10;
  int eyeY = cy - 10;
  int eyeLX = cx - eyeSpacing;
  int eyeRX = cx + eyeSpacing;
  int eyeR  = 7;   // eye radius

  switch (p.expr) {
    case EXPR_NORMAL:
      // Dark ring
      tft.fillCircle(eyeLX, eyeY, eyeR + 1, LB_DCYAN);
      tft.fillCircle(eyeRX, eyeY, eyeR + 1, LB_DCYAN);
      // Cyan fill
      tft.fillCircle(eyeLX, eyeY, eyeR, LB_CYAN);
      tft.fillCircle(eyeRX, eyeY, eyeR, LB_CYAN);
      // Pupils (dark inner)
      tft.fillCircle(eyeLX + d * 2, eyeY, 3, LB_BLACK);
      tft.fillCircle(eyeRX + d * 2, eyeY, 3, LB_BLACK);
      // White highlight (sparkle)
      tft.fillCircle(eyeLX - 2, eyeY - 3, 2, LB_WHITE);
      tft.fillCircle(eyeRX - 2, eyeY - 3, 2, LB_WHITE);
      break;

    case EXPR_HAPPY:
      // Happy squint — arc eyes (upside-down U)
      tft.fillCircle(eyeLX, eyeY, eyeR, LB_CYAN);
      tft.fillCircle(eyeRX, eyeY, eyeR, LB_CYAN);
      // Cover bottom half to make happy squint
      tft.fillRect(eyeLX - eyeR, eyeY, eyeR * 2, eyeR + 2, LB_RED);
      tft.fillRect(eyeRX - eyeR, eyeY, eyeR * 2, eyeR + 2, LB_RED);
      // Add a thin arc line
      tft.drawCircleHelper(eyeLX, eyeY + 1, eyeR - 1, 0x03, LB_DCYAN);
      tft.drawCircleHelper(eyeRX, eyeY + 1, eyeR - 1, 0x03, LB_DCYAN);
      break;

    case EXPR_ALERT:
      // Wide eyes — bigger pupils, raised position
      eyeY -= 2;
      tft.fillCircle(eyeLX, eyeY, eyeR + 2, LB_DCYAN);
      tft.fillCircle(eyeRX, eyeY, eyeR + 2, LB_DCYAN);
      tft.fillCircle(eyeLX, eyeY, eyeR + 1, LB_CYAN);
      tft.fillCircle(eyeRX, eyeY, eyeR + 1, LB_CYAN);
      // Small pupils = surprised look
      tft.fillCircle(eyeLX + d, eyeY, 2, LB_BLACK);
      tft.fillCircle(eyeRX + d, eyeY, 2, LB_BLACK);
      tft.fillCircle(eyeLX - 2, eyeY - 3, 2, LB_WHITE);
      tft.fillCircle(eyeRX - 2, eyeY - 3, 2, LB_WHITE);
      break;

    case EXPR_SLEEPY:
      // Half-closed eyes — horizontal line with small gap
      tft.fillCircle(eyeLX, eyeY + 2, eyeR, LB_DCYAN);
      tft.fillCircle(eyeRX, eyeY + 2, eyeR, LB_DCYAN);
      // Cover top 2/3 with body color = droopy
      tft.fillRect(eyeLX - eyeR - 1, eyeY - eyeR, eyeR * 2 + 2, eyeR + 4, LB_RED);
      tft.fillRect(eyeRX - eyeR - 1, eyeY - eyeR, eyeR * 2 + 2, eyeR + 4, LB_RED);
      break;

    case EXPR_DIZZY:
      // Offset eyes + spiral suggestion
      int dOff = (f % 8) - 4;  // wobble
      tft.fillCircle(eyeLX + dOff, eyeY, eyeR, LB_CYAN);
      tft.fillCircle(eyeRX - dOff, eyeY, eyeR, LB_CYAN);
      // X eyes for dizzy
      tft.drawLine(eyeLX - 3, eyeY - 3, eyeLX + 3, eyeY + 3, LB_BLACK);
      tft.drawLine(eyeLX + 3, eyeY - 3, eyeLX - 3, eyeY + 3, LB_BLACK);
      tft.drawLine(eyeRX - 3, eyeY - 3, eyeRX + 3, eyeY + 3, LB_BLACK);
      tft.drawLine(eyeRX + 3, eyeY - 3, eyeRX - 3, eyeY + 3, LB_BLACK);
      break;
  }

  // ═══════════════════════════════════════
  //  ANTENNAE — two stalks from top of head
  // ═══════════════════════════════════════
  int aWave1 = ((f / 3) % 6) - 3;   // gentle wave
  int aWave2 = ((f / 4) % 5) - 2;

  int a1BaseX = cx - 6;
  int a1BaseY = cy - 18;
  int a1TipX  = cx - 14 + aWave1;
  int a1TipY  = cy - 38 + abs(aWave2);

  int a2BaseX = cx + 6;
  int a2BaseY = cy - 18;
  int a2TipX  = cx + 14 + aWave2;
  int a2TipY  = cy - 36 + abs(aWave1);

  // Draw antennae as thick lines (2px)
  tft.drawLine(a1BaseX, a1BaseY, a1TipX, a1TipY, LB_RED);
  tft.drawLine(a1BaseX + 1, a1BaseY, a1TipX + 1, a1TipY, LB_RED);
  tft.drawLine(a2BaseX, a2BaseY, a2TipX, a2TipY, LB_RED);
  tft.drawLine(a2BaseX + 1, a2BaseY, a2TipX + 1, a2TipY, LB_RED);

  // Antenna tips (small bulbs)
  tft.fillCircle(a1TipX, a1TipY, 3, LB_GOLD);
  tft.fillCircle(a2TipX, a2TipY, 3, LB_GOLD);

  // ═══════════════════════════════════════
  //  CLAWS — large pincers on sides
  // ═══════════════════════════════════════
  int clawOpen = 0;
  int clawY = cy - 2;

  switch (p.clawPose) {
    case CLAW_IDLE:
      clawOpen = (f % 30 < 15) ? 2 : 0;  // slow idle pinch
      _drawClaw(cx - 30, clawY, -1, clawOpen, f);
      _drawClaw(cx + 30, clawY,  1, clawOpen, f);
      break;

    case CLAW_RAISED:
      clawOpen = (f % 10 < 5) ? 4 : 1;  // excited snapping
      _drawClaw(cx - 28, clawY - 10, -1, clawOpen, f);
      _drawClaw(cx + 28, clawY - 10,  1, clawOpen, f);
      break;

    case CLAW_HOLDING:
      // Left claw normal, right claw holds item
      clawOpen = (f % 30 < 15) ? 2 : 0;
      _drawClaw(cx - 30, clawY, -1, clawOpen, f);
      _drawClawHolding(cx + 28, clawY - 4, 1, p.heldIcon, p.heldColor, f);
      break;

    case CLAW_TUCKED:
      // Small, tucked-in claws close to body
      _drawClawTucked(cx - 18, clawY + 4, -1);
      _drawClawTucked(cx + 18, clawY + 4,  1);
      break;
  }

  // ═══════════════════════════════════════
  //  ARMS — connect body to claws
  // ═══════════════════════════════════════
  int armYtop = cy - 4;
  int armYbot = cy + 2;
  // Left arm
  tft.fillRect(cx - 24, armYtop, 6, armYbot - armYtop, LB_RED);
  // Right arm
  tft.fillRect(cx + 18, armYtop, 6, armYbot - armYtop, LB_RED);

  // ═══════════════════════════════════════
  //  LEGS — 3 pairs underneath
  // ═══════════════════════════════════════
  for (int i = 0; i < 3; i++) {
    int lx = cx - 8 + i * 8;
    int anim = ((f / 4 + i) % 3);
    int legLen = 8 + anim * 2;
    // Left leg
    tft.drawLine(lx, cy + 16, lx - 4 - anim, cy + 16 + legLen, LB_DRED);
    // Right side mirror
    int rx = cx + 8 - i * 8;
    tft.drawLine(rx, cy + 16, rx + 4 + anim, cy + 16 + legLen, LB_DRED);
  }
}

// ────────────────────────────────────────────
//  CLAW DRAWING HELPERS
// ────────────────────────────────────────────

// Regular claw: two pincers with gap
inline void _drawClaw(int cx, int cy, int side, int openAmt, int f) {
  // Upper pincer
  tft.fillCircle(cx, cy - 4 - openAmt, 6, LB_GOLD);
  tft.fillCircle(cx + side * 4, cy - 4 - openAmt, 4, LB_GOLD);
  // Lower pincer
  tft.fillCircle(cx, cy + 4 + openAmt, 6, LB_GOLD);
  tft.fillCircle(cx + side * 4, cy + 4 + openAmt, 4, LB_GOLD);
  // Pincer tips (darker)
  tft.fillCircle(cx + side * 7, cy - 4 - openAmt, 2, LB_DGOLD);
  tft.fillCircle(cx + side * 7, cy + 4 + openAmt, 2, LB_DGOLD);
  // Joint connecting to arm
  tft.fillCircle(cx - side * 4, cy, 4, LB_RED);
}

// Claw holding an item: closed pincer + item icon floating above
inline void _drawClawHolding(int cx, int cy, int side, const uint8_t* icon, uint16_t color, int f) {
  // Closed pincer (holding position)
  tft.fillCircle(cx, cy - 2, 6, LB_GOLD);
  tft.fillCircle(cx + side * 4, cy - 2, 4, LB_GOLD);
  tft.fillCircle(cx, cy + 2, 6, LB_GOLD);
  tft.fillCircle(cx + side * 4, cy + 2, 4, LB_GOLD);
  tft.fillCircle(cx + side * 7, cy - 2, 2, LB_DGOLD);
  tft.fillCircle(cx + side * 7, cy + 2, 2, LB_DGOLD);
  // Joint
  tft.fillCircle(cx - side * 4, cy, 4, LB_RED);

  // Draw held item floating above claw
  if (icon != NULL) {
    int iconX = cx + side * 2 - 8;  // center 16px icon on claw
    int iconY = cy - 22;            // above claw
    drawIcon16(iconX, iconY, icon, color);
  }
}

// Tucked claw: small, close to body
inline void _drawClawTucked(int cx, int cy, int side) {
  tft.fillCircle(cx, cy, 4, LB_GOLD);
  tft.fillCircle(cx + side * 3, cy - 1, 3, LB_GOLD);
  tft.fillCircle(cx + side * 3, cy + 1, 3, LB_GOLD);
}

// ────────────────────────────────────────────
//  BOUNDING BOX (for erase optimization)
//  Returns the pixel bounds of the lobster
// ────────────────────────────────────────────
inline void getLobsterBounds(const LobsterPose& p, int& left, int& top, int& right, int& bottom) {
  int cx = p.cx + (int)p.swayOffset;
  int cy = p.cy + (int)p.bobOffset;
  left   = cx - 42;   // claws extend ~30 + margin
  right  = cx + 42;
  top    = cy - 42;   // antennae tip
  bottom = cy + 30;   // legs
}


// ── SCENARIOS_V2.H (scenario lookup + renderer) ──
/*
 * ╔═════════════════════════════════════════╗
 * ║  scenarios_v2.h — Scenario → Lobster    ║
 * ║  Maps 23 scenarios to lobster pose,     ║
 * ║  expression, claw state, and large icon ║
 * ╚═════════════════════════════════════════╝
 *
 * This replaces the small notification card with:
 * 1. Large centered icon (40-50px, procedural)
 * 2. Message text below icon
 * 3. Lobster expression/claw change per scenario
 */

// still used for 16×16 held items

// ────────────────────────────────────────────
//  Scenario → Lobster mapping
// ────────────────────────────────────────────

// Icon draw function pointer type




// ────────────────────────────────────────────
//  Scenario Enum (re-use from scenarios.h)
// ────────────────────────────────────────────


// ────────────────────────────────────────────
//  Lookup Table
// ────────────────────────────────────────────
static const ScenarioV2Config SCENARIO_V2_TABLE[] = {
  // S2_IDLE — no icon, normal idle lobster
  { NULL,         NULL,                     0x7BEF, EXPR_NORMAL, CLAW_IDLE,    NULL, 0 },
  // S2_FOCUS — shield icon, tucked claws, sleepy/calm
  { "Focus",      drawIconLargeShield,      0x07E0, EXPR_SLEEPY, CLAW_TUCKED,  NULL, 0 },
  // S2_CONTEXT_SWITCH — warning, dizzy expression
  { "Too fast",   drawIconLargeWarning,     0xFD20, EXPR_DIZZY,  CLAW_RAISED,  NULL, 0 },
  // S2_TASK_NUDGE — question mark, holding question icon
  { "This?",      drawIconLargeQuestion,    0xFE8C, EXPR_NORMAL, CLAW_HOLDING, ICON_QUESTION, 0xFE8C },
  // S2_LONG_SESSION — clock, alert eyes
  { "Break?",     drawIconLargeClock,       0xFD20, EXPR_ALERT,  CLAW_IDLE,    NULL, 0 },
  // S2_MEETING_SOON — clock, alert
  { "5 min",      drawIconLargeClock,       0x2C9F, EXPR_ALERT,  CLAW_RAISED,  NULL, 0 },
  // S2_LATE — big warning, urgent
  { "Late!",      drawIconLargeWarning,     0xE1C8, EXPR_ALERT,  CLAW_RAISED,  NULL, 0 },
  // S2_EMAIL — envelope, holding envelope
  { "Mail!",      drawIconLargeEnvelope,    0x2C9F, EXPR_ALERT,  CLAW_HOLDING, ICON_ENVELOPE, 0x2C9F },
  // S2_TEAMS_MSG — chat bubble, holding chat
  { "Ping!",      drawIconLargeChat,        0x541F, EXPR_ALERT,  CLAW_HOLDING, ICON_CHAT, 0x541F },
  // S2_WORTH_REPLY — reply arrow, holding reply
  { "Reply?",     drawIconLargeReply,       0x2C9F, EXPR_NORMAL, CLAW_HOLDING, ICON_REPLY, 0x2C9F },
  // S2_QUIET_TOO_LONG — eye icon, normal
  { "Silent...",  drawIconLargeEye,         0x7BEF, EXPR_NORMAL, CLAW_IDLE,    NULL, 0 },
  // S2_MOVE — runner, alert
  { "Move",       drawIconLargeRunner,      0xFD20, EXPR_ALERT,  CLAW_RAISED,  NULL, 0 },
  // S2_HYDRATE — droplet, holding droplet
  { "Water",      drawIconLargeDroplet,     0x07FF, EXPR_NORMAL, CLAW_HOLDING, ICON_DROPLET, 0x07FF },
  // S2_MORNING — sunrise, happy
  { "Top 1?",     drawIconLargeSunrise,     0xFE8C, EXPR_HAPPY,  CLAW_RAISED,  NULL, 0 },
  // S2_SHUTDOWN — moon, sleepy
  { "Done?",      drawIconLargeMoon,        0x7BEF, EXPR_SLEEPY, CLAW_TUCKED,  NULL, 0 },
  // S2_WORKING — gear, holding gear
  { "Working",    drawIconLargeGear,        0x7BEF, EXPR_NORMAL, CLAW_HOLDING, ICON_GEAR, 0x7BEF },
  // S2_DONE — checkmark, happy + raised claws
  { "Done",       drawIconLargeCheck,       0x07E0, EXPR_HAPPY,  CLAW_RAISED,  NULL, 0 },
  // S2_NEEDS_INPUT — question, alert
  { "You?",       drawIconLargeQuestion,    0xFE8C, EXPR_ALERT,  CLAW_HOLDING, ICON_QUESTION, 0xFE8C },
  // S2_STUCK — warning, dizzy
  { "Stuck?",     drawIconLargeWarning,     0xFD20, EXPR_DIZZY,  CLAW_IDLE,    NULL, 0 },
  // S2_DECISION — scales, holding scales
  { "A / B",      drawIconLargeScales,      0xFE8C, EXPR_NORMAL, CLAW_HOLDING, ICON_SCALES, 0xFE8C },
  // S2_DOOMSCROLL — eye, sleepy
  { "Hmm...",     drawIconLargeEye,         0x7BEF, EXPR_SLEEPY, CLAW_IDLE,    NULL, 0 },
  // S2_CELEBRATION — star, happy + raised
  { "Nice.",      drawIconLargeStar,        0x07E0, EXPR_HAPPY,  CLAW_RAISED,  NULL, 0 },
  // S2_IDLE_PERSONALITY — no message, random expression
  { NULL,         NULL,                     0x7BEF, EXPR_HAPPY,  CLAW_IDLE,    NULL, 0 },
};

inline const ScenarioV2Config& getScenarioV2(ScenarioV2 sc) {
  if (sc >= S2_COUNT) sc = S2_IDLE;
  return SCENARIO_V2_TABLE[sc];
}

// ────────────────────────────────────────────
//  NOTIFICATION LAYOUT RENDERER
//  Layout: Large icon centered → message below
//  Position: in the middle zone between clock and lobster
// ────────────────────────────────────────────

// Notification area Y range (below clock, above lobster)
#define NOTIF_ICON_CY  (SEG_Y0 + SEG_DH + 50)  // icon center Y
#define NOTIF_TEXT_Y   (NOTIF_ICON_CY + 30)      // text Y below icon

inline void renderNotificationV2(ScenarioV2 sc) {
  const ScenarioV2Config& cfg = getScenarioV2(sc);
  if (cfg.message == NULL && cfg.drawIcon == NULL) return;

  int iconCX = SW / 2;
  int iconCY = NOTIF_ICON_CY;

  // Draw the large icon
  if (cfg.drawIcon != NULL) {
    cfg.drawIcon(iconCX, iconCY);
  }

  // Draw message text centered below icon
  if (cfg.message != NULL) {
    int textW = strlen(cfg.message) * 12;  // size 2 = 12px per char
    int textX = (SW - textW) / 2;
    tft.setTextSize(2);
    tft.setTextColor(cfg.accentColor, 0x18C5);  // text on BG
    tft.setCursor(textX, NOTIF_TEXT_Y);
    tft.print(cfg.message);
  }
}

// Clear the notification area
inline void clearNotificationV2() {
  int y0 = NOTIF_ICON_CY - 30;
  int h  = (NOTIF_TEXT_Y + 20) - y0;
  tft.fillRect(0, y0, SW, h, 0x18C5);
}

// ────────────────────────────────────────────
//  Apply scenario to lobster pose
// ────────────────────────────────────────────
inline void applyScenarioToLobster(ScenarioV2 sc, LobsterPose& pose) {
  const ScenarioV2Config& cfg = getScenarioV2(sc);
  pose.expr      = cfg.expression;
  pose.clawPose  = cfg.clawPose;
  pose.heldIcon  = cfg.heldItem;
  pose.heldColor = cfg.heldColor;
}


// ── SKETCH BODY ──
#endif

// ────────────────────────────────────────────
//  RGB565 COLOR PALETTE
// ────────────────────────────────────────────
#define C_BG       0x18C5   // dark navy  #1A1A2E
#define C_WHITE    0xFFFF
#define C_DIMWH    0x31A6   // dim colon
#define C_GRAY     0x7BEF
#define C_LGRAY    0xAD75
#define C_RED      0xE1C8   // lobster red #E63946
#define C_DRED     0xC083   // shadow red  #C1121F
#define C_GOLD     0xFE8C   // claw gold   #FFD166
#define C_BLACK    0x0000
#define C_GROUND   0x2104   // subtle ground
#define C_STATUS   0x04A0   // teal status dot
#define C_WARN     0xFD20   // orange for urgent
#define C_NOTIF    0x2C9F   // blue for notifications
#define C_TEAMS    0x541F   // purple for Teams

// ════════════════════════════════════════════
//  CLOCK STATE
// ════════════════════════════════════════════
unsigned long tBase;
long simSec;
int prevH = -1, prevM = -1, prevS = -1;
bool colonVis = true;
bool timeFromNTP = false;

// ════════════════════════════════════════════
//  WIFI STATE
// ════════════════════════════════════════════
bool wifiConnected = false;
unsigned long lastPoll = 0;
unsigned long lastWifiCheck = 0;

// ════════════════════════════════════════════
//  NOTIFICATION STATE
// ════════════════════════════════════════════
#define MAX_NOTIFS 5
struct Notification {
  char type[16];     // "email", "teams", "calendar", "urgent"
  char title[64];
  char body[128];
  char timeStr[8];   // "09:41"
  bool active;
};
Notification notifs[MAX_NOTIFS];
int notifCount = 0;
bool showingNotif = false;
unsigned long notifShowTime = 0;
int currentNotif = 0;

// ════════════════════════════════════════════
//  SCENARIO STATE
// ════════════════════════════════════════════
Scenario currentScenario = SC_IDLE;

// ════════════════════════════════════════════
//  LOBSTER STATE MACHINE
// ════════════════════════════════════════════
enum LState { SIT, GO_R, GO_L, LOOK };
struct {
  float x;
  int y;
  LState st;
  LMood mood;
  int fr;
  int dir;
  unsigned long nextSt;
  unsigned long nextFr;
} L;

// Bounding box of previous frame (for erase)
int pL = 0, pR = 0, pT = 0, pB = 0;

// Ground Y position
#define GROUND_Y (SH - 34)
#define LOB_Y    (SH - 18)

// ════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0) ^ micros());

  // Backlight on (real hardware)
  #ifdef PLATFORM_WAVESHARE
    if (TFT_BL >= 0) {
      pinMode(TFT_BL, OUTPUT);
      analogWrite(TFT_BL, 200);  // ~80% brightness
    }
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
    tft.init(172, 320);
  #else
    tft.begin();
  #endif

  tft.setRotation(0);
  tft.fillScreen(C_BG);

  // Dotted ground line
  drawGroundFull();

  // Status: starting
  drawStatus("booting", C_GRAY);

  // Clock (start at 12:00:00 — will be replaced by NTP)
  tBase = millis();
  simSec = 12L * 3600;
  seg7_init();
  seg7_renderTime(12, 0, true);
  seg7_renderSec(0);
  seg7_renderDate("Thu, May 1");

  // Version label
  tft.setTextSize(1);
  tft.setTextColor(C_GROUND);
  tft.setCursor(SW - 60, SH - 10);
  tft.print("Red v0.2");

  // Lobster init
  L.x = SW / 2;
  L.y = LOB_Y;
  L.st = SIT;
  L.mood = IDLE;
  L.fr = 0;
  L.dir = 1;
  L.nextSt = millis() + random(3000, 7000);
  L.nextFr = millis();

  Serial.println("[Red Device] ClawMagotchi v0.2 booting...");

  // WiFi + NTP (real hardware only)
  #ifdef PLATFORM_WAVESHARE
    connectWiFi();
    if (wifiConnected) {
      configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
      Serial.println("[NTP] Time sync requested");
    }
  #endif

  drawStatus(wifiConnected ? "online" : "offline", wifiConnected ? C_STATUS : C_GRAY);
  Serial.println("[Red Device] Ready!");
}

// ════════════════════════════════════════════
//  MAIN LOOP
// ════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // Clock: tick every 500ms (colon blink)
  static unsigned long lastTick = 0;
  if (now - lastTick >= 500) {
    lastTick = now;
    tickClock(now);
  }

  // Lobster: ~12 fps
  if (now >= L.nextFr) {
    L.nextFr = now + 83;
    tickLobster(now);
  }

  // Notification display timeout (show for 8 sec, then return to clock)
  if (showingNotif && (now - notifShowTime > 8000)) {
    dismissNotification();
  }

  #ifdef PLATFORM_WAVESHARE
    // Poll for notifications
    if (wifiConnected && (now - lastPoll > POLL_INTERVAL)) {
      lastPoll = now;
      pollNotifications();
    }

    // WiFi reconnect check every 30s
    if (!wifiConnected && (now - lastWifiCheck > 30000)) {
      lastWifiCheck = now;
      connectWiFi();
      drawStatus(wifiConnected ? "online" : "offline", wifiConnected ? C_STATUS : C_GRAY);
    }
  #endif

  // Also accept notifications via Serial (works on both platforms)
  checkSerial();
}

// ════════════════════════════════════════════
//  CLOCK RENDERING
// ════════════════════════════════════════════

void tickClock(unsigned long now) {
  int h, m, s;

  #ifdef PLATFORM_WAVESHARE
    if (timeFromNTP || wifiConnected) {
      struct tm ti;
      if (getLocalTime(&ti, 100)) {
        h = ti.tm_hour;
        m = ti.tm_min;
        s = ti.tm_sec;
        timeFromNTP = true;
      } else {
        // Fallback to simulated time
        long t = (simSec + (now - tBase) / 1000) % 86400;
        h = t / 3600;
        m = (t % 3600) / 60;
        s = t % 60;
      }
    } else
  #endif
  {
    long t = (simSec + (now - tBase) / 1000) % 86400;
    h = t / 3600;
    m = (t % 3600) / 60;
    s = t % 60;
  }

  colonVis = !colonVis;

  if (h != prevH || m != prevM) {
    seg7_renderTime(h, m, colonVis);
    prevH = h;
    prevM = m;
  } else {
    seg7_renderTime(prevH, prevM, colonVis);
  }

  if (s != prevS) {
    seg7_renderSec(s);
    prevS = s;

    // Update date from NTP once per minute
    #ifdef PLATFORM_WAVESHARE
    if (s == 0 && timeFromNTP) {
      struct tm ti;
      if (getLocalTime(&ti, 100)) {
        const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        const char* mons[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        char buf[20];
        snprintf(buf, sizeof(buf), "%s, %s %d", days[ti.tm_wday], mons[ti.tm_mon], ti.tm_mday);
        seg7_renderDate(buf);
      }
    }
    #endif
  }
}

// Adaptive text sizing for narrow screens
#define CLOCK_SIZE  ((SW >= 200) ? 6 : 5)
#define CLOCK_CW    ((SW >= 200) ? 36 : 30)
#define CLOCK_Y     ((SW >= 200) ? 55 : 55)

void renderTime(int h, int m, bool col) {
  const int y = CLOCK_Y;
  const int cw = CLOCK_CW;
  int x0 = (SW - cw * 5) / 2;

  tft.setTextSize(CLOCK_SIZE);
  tft.setTextColor(C_WHITE, C_BG);

  tft.setCursor(x0, y);
  if (h < 10) tft.print('0');
  tft.print(h);

  renderColon(col);

  tft.setCursor(x0 + cw * 3, y);
  if (m < 10) tft.print('0');
  tft.print(m);
}

void renderColon(bool on) {
  const int y = CLOCK_Y;
  const int cw = CLOCK_CW;
  int x0 = (SW - cw * 5) / 2;

  tft.setTextSize(CLOCK_SIZE);
  tft.setTextColor(on ? C_WHITE : C_DIMWH, C_BG);
  tft.setCursor(x0 + cw * 2, y);
  tft.print(':');
}

void renderSec(int s) {
  int x = (SW - 24) / 2;
  tft.setTextSize(2);
  tft.setTextColor(C_LGRAY, C_BG);
  tft.setCursor(x, CLOCK_Y + (CLOCK_SIZE * 8) + 4);
  if (s < 10) tft.print('0');
  tft.print(s);
}

void renderDate() {
  const char* d = "Thu, May 1";
  int w = strlen(d) * 12;
  tft.setTextSize(2);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setCursor((SW - w) / 2, CLOCK_Y + (CLOCK_SIZE * 8) + 28);
  tft.print(d);
}

#ifdef PLATFORM_WAVESHARE
void renderDateNTP() {
  struct tm ti;
  if (!getLocalTime(&ti, 100)) return;
  const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  const char* mons[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  char buf[20];
  snprintf(buf, sizeof(buf), "%s, %s %d", days[ti.tm_wday], mons[ti.tm_mon], ti.tm_mday);
  int w = strlen(buf) * 12;
  int dateY = CLOCK_Y + (CLOCK_SIZE * 8) + 28;
  tft.fillRect(0, dateY, SW, 18, C_BG);
  tft.setTextSize(2);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setCursor((SW - w) / 2, dateY);
  tft.print(buf);
}
#endif

// ════════════════════════════════════════════
//  GROUND
// ════════════════════════════════════════════

void drawGroundFull() {
  for (int i = 0; i < SW; i += 2) {
    tft.drawPixel(i, GROUND_Y, C_GROUND);
  }
}

void drawGroundRange(int x1, int x2) {
  x1 = max(x1, 0);
  x2 = min(x2, SW);
  for (int i = x1; i < x2; i += 2) {
    tft.drawPixel(i, GROUND_Y, C_GROUND);
  }
}

// ════════════════════════════════════════════
//  LOBSTER 🦞  — State Machine & Animation
// ════════════════════════════════════════════

void tickLobster(unsigned long now) {
  // Erase previous frame
  if (pR > pL) {
    tft.fillRect(pL, pT, pR - pL, pB - pT, C_BG);
    // Restore ground dots inside erased area
    if (pT <= GROUND_Y && pB >= GROUND_Y) {
      drawGroundRange(pL, pR);
    }
  }

  // ── State transitions ──
  if (now >= L.nextSt) {
    int r = random(100);

    if (L.st == LOOK) {
      // After looking around, pick a new action
      if (r < 40)      { L.st = SIT;  L.nextSt = now + random(3000, 7000); }
      else if (r < 70) { L.st = GO_R; L.dir = 1;  L.nextSt = now + random(2000, 5000); }
      else             { L.st = GO_L; L.dir = -1; L.nextSt = now + random(2000, 5000); }
    }
    else if (r < 30) {
      L.st = SIT;
      L.nextSt = now + random(3000, 8000);
    }
    else if (r < 50) {
      L.st = GO_R;
      L.dir = 1;
      L.nextSt = now + random(2000, 5000);
    }
    else if (r < 70) {
      L.st = GO_L;
      L.dir = -1;
      L.nextSt = now + random(2000, 5000);
    }
    else {
      L.st = LOOK;
      L.nextSt = now + random(1500, 3000);
    }
  }

  // ── Movement ──
  float speed = 0.8;
  if (L.st == GO_R) {
    L.x += speed;
    if (L.x > SW - 30) { L.x = SW - 30; L.st = SIT; L.nextSt = now + 2000; }
  }
  else if (L.st == GO_L) {
    L.x -= speed;
    if (L.x < 30) { L.x = 30; L.st = SIT; L.nextSt = now + 2000; }
  }
  else if (L.st == LOOK) {
    if (L.fr % 10 == 0) L.dir = -L.dir;
  }

  L.fr++;

  // ── Draw ──
  drawLob((int)L.x, L.y, L.fr, L.dir);
}

// ────────────────────────────────────────────
//  DRAW THE LOBSTER
//
//  Procedural pixel art using drawing primitives.
//  cx,cy = center of body. d = facing direction.
//  All coordinates computed relative to center.
// ────────────────────────────────────────────

void drawLob(int cx, int cy, int f, int d) {
  // Store bounding box for next erase
  pL = cx - 27;
  pR = cx + 27;
  pT = cy - 23;
  pB = cy + 13;

  // ── TAIL ──
  // Fan (triangle at end)
  int ftx = cx - d * 22;   // fan tip
  int fbx = cx - d * 15;   // fan base
  tft.fillTriangle(fbx, cy - 3, fbx, cy + 4, ftx, cy, C_DRED);
  // Segments
  tft.fillCircle(cx - d * 12, cy, 4, C_RED);
  tft.fillCircle(cx - d * 7,  cy + 1, 5, C_RED);

  // ── BODY ──
  tft.fillRoundRect(cx - 8, cy - 5, 16, 11, 5, C_RED);
  // Belly shadow
  tft.drawFastHLine(cx - 5, cy + 5, 10, C_DRED);

  // ── HEAD ──
  tft.fillCircle(cx + d * 8, cy - 1, 6, C_RED);

  // ── EYE ──
  bool blink = (f % 80 > 75);
  if (!blink) {
    tft.fillCircle(cx + d * 10, cy - 3, 2, C_WHITE);
    tft.fillCircle(cx + d * 11, cy - 3, 1, C_BLACK);
  } else {
    tft.drawFastHLine(cx + d * 9, cy - 3, 3, C_WHITE);
  }

  // ── CLAWS ──
  int co = (f % 16 < 8) ? 0 : 2;  // open/close cycle
  int clawX = cx + d * 18;

  // Arm connecting head to claws
  int ax1 = cx + d * 8;
  int ax2 = cx + d * 14;
  int armL = (d > 0) ? ax1 : ax2;
  int armW = abs(ax2 - ax1) + 1;
  tft.fillRect(armL, cy - 4, armW, 3, C_RED);

  // Upper pincer
  tft.fillCircle(clawX, cy - 5 - co, 3, C_GOLD);
  // Lower pincer
  tft.fillCircle(clawX, cy + 0 + co, 3, C_GOLD);
  // Pincer tips
  tft.drawPixel(clawX + d * 3, cy - 5 - co, C_DRED);
  tft.drawPixel(clawX + d * 3, cy + 0 + co, C_DRED);

  // ── ANTENNAE ──
  int w1 = ((f / 3) % 4 < 2) ? -2 : 2;
  int w2 = ((f / 4) % 3 < 2) ?  2 : -2;
  int a1x = cx + d * 16, a1y = cy - 19 + w1;
  int a2x = cx + d * 12, a2y = cy - 21 + w2;
  tft.drawLine(cx + d * 7, cy - 6, a1x, a1y, C_GOLD);
  tft.drawLine(cx + d * 5, cy - 7, a2x, a2y, C_GOLD);
  // Antenna tips (tiny dots)
  tft.fillCircle(a1x, a1y, 1, C_GOLD);
  tft.fillCircle(a2x, a2y, 1, C_GOLD);

  // ── LEGS ──
  for (int i = 0; i < 4; i++) {
    int lx = cx - d * 3 + d * i * 3;
    int anim = ((f / 3 + i) % 2) * 2;
    tft.drawLine(lx, cy + 5, lx - d, cy + 9 + anim, C_DRED);
  }
}

// ════════════════════════════════════════════
//  STATUS INDICATOR
// ════════════════════════════════════════════

void drawStatus(const char* label, uint16_t color) {
  int y = CLOCK_Y + (CLOCK_SIZE * 8) + 52;
  tft.fillRect(0, y - 2, SW, 22, C_BG);
  tft.fillCircle(SW / 2 - 20, y + 5, 3, color);
  tft.setTextSize(1);
  tft.setTextColor(C_GROUND, C_BG);
  tft.setCursor(SW / 2 - 12, y + 2);
  tft.print(label);
}

// ════════════════════════════════════════════
//  WiFi CONNECTION
// ════════════════════════════════════════════

#ifdef PLATFORM_WAVESHARE
void connectWiFi() {
  Serial.printf("[WiFi] Connecting to %s...\n", WIFI_SSID);
  drawStatus("connecting", C_GOLD);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < WIFI_TIMEOUT)) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    wifiConnected = false;
    Serial.println("[WiFi] Connection failed");
  }
}

// ════════════════════════════════════════════
//  NOTIFICATION POLLING (HTTP)
// ════════════════════════════════════════════

void pollNotifications() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    return;
  }

  HTTPClient http;
  String url = String("http://") + API_HOST + ":" + API_PORT + API_PATH;
  http.begin(url);
  http.setTimeout(3000);

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    parseNotifications(payload);
  } else if (code > 0) {
    Serial.printf("[API] HTTP %d\n", code);
  } else {
    Serial.printf("[API] Error: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

void parseNotifications(const String& json) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[API] JSON parse error: %s\n", err.c_str());
    return;
  }

  JsonArray arr = doc["notifications"].as<JsonArray>();
  notifCount = 0;

  for (JsonObject obj : arr) {
    if (notifCount >= MAX_NOTIFS) break;
    Notification& n = notifs[notifCount];
    strlcpy(n.type,    obj["type"]  | "info",    sizeof(n.type));
    strlcpy(n.title,   obj["title"] | "",        sizeof(n.title));
    strlcpy(n.body,    obj["body"]  | "",        sizeof(n.body));
    strlcpy(n.timeStr, obj["time"]  | "",        sizeof(n.timeStr));
    n.active = true;
    notifCount++;
  }

  if (notifCount > 0 && !showingNotif) {
    currentNotif = 0;
    showNotification(currentNotif);
    L.mood = (strcmp(notifs[0].type, "urgent") == 0) ? URGENT : ALERT;
  }
}
#endif

// ════════════════════════════════════════════
//  SERIAL COMMAND INTERFACE
//  Send JSON via Serial for testing on any platform:
//  {"type":"email","title":"Chris Luce","body":"FY27 budget review"}
// ════════════════════════════════════════════

void checkSerial() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  Serial.printf("[Serial] Received: %s\n", line.c_str());

  // Simple JSON parse for notification
  if (line.startsWith("{")) {
    #ifdef PLATFORM_WAVESHARE
      parseNotifications(String("{\"notifications\":[") + line + "]}");
    #else
      // Minimal parse for Wokwi (no ArduinoJson)
      parseSerialNotif(line);
    #endif
  }
  else if (line == "dismiss") {
    dismissNotification();
  }
  else if (line == "status") {
    Serial.printf("[Status] WiFi=%s Notifs=%d Mood=%d\n",
      wifiConnected ? "yes" : "no", notifCount, L.mood);
  }
}

#ifndef PLATFORM_WAVESHARE
// Lightweight serial notification parser for Wokwi (no ArduinoJson)
void parseSerialNotif(const String& json) {
  Notification& n = notifs[0];
  n.active = true;
  notifCount = 1;

  // Extract type
  int ti = json.indexOf("\"type\"");
  if (ti >= 0) {
    int q1 = json.indexOf('\"', json.indexOf(':', ti) + 1);
    int q2 = json.indexOf('\"', q1 + 1);
    if (q1 >= 0 && q2 > q1) json.substring(q1 + 1, q2).toCharArray(n.type, sizeof(n.type));
  } else {
    strcpy(n.type, "info");
  }

  // Extract title
  ti = json.indexOf("\"title\"");
  if (ti >= 0) {
    int q1 = json.indexOf('\"', json.indexOf(':', ti) + 1);
    int q2 = json.indexOf('\"', q1 + 1);
    if (q1 >= 0 && q2 > q1) json.substring(q1 + 1, q2).toCharArray(n.title, sizeof(n.title));
  } else {
    strcpy(n.title, "Notification");
  }

  // Extract body
  ti = json.indexOf("\"body\"");
  if (ti >= 0) {
    int q1 = json.indexOf('\"', json.indexOf(':', ti) + 1);
    int q2 = json.indexOf('\"', q1 + 1);
    if (q1 >= 0 && q2 > q1) json.substring(q1 + 1, q2).toCharArray(n.body, sizeof(n.body));
  } else {
    strcpy(n.body, "");
  }

  strcpy(n.timeStr, "");
  showNotification(0);
  L.mood = (strcmp(n.type, "urgent") == 0) ? URGENT : ALERT;
}
#endif

// ════════════════════════════════════════════
//  NOTIFICATION DISPLAY
// ════════════════════════════════════════════

void showNotification(int idx) {
  if (idx >= notifCount) return;
  Notification& n = notifs[idx];

  showingNotif = true;
  notifShowTime = millis();

  // Map notification type to scenario
  if (strcmp(n.type, "email") == 0)         currentScenario = SC_EMAIL;
  else if (strcmp(n.type, "teams") == 0)    currentScenario = SC_TEAMS_MSG;
  else if (strcmp(n.type, "calendar") == 0) currentScenario = SC_MEETING_SOON;
  else if (strcmp(n.type, "urgent") == 0)   currentScenario = SC_LATE;
  else                                      currentScenario = SC_NEEDS_INPUT;

  // Render scenario card (icon badge + short message)
  renderNotificationV2((ScenarioV2)currentScenario);

  // Notification card area (below scenario card, for detail text)
  int cardY = CLOCK_Y + (CLOCK_SIZE * 8) + 20;
  int cardH = 70;

  // Pick color based on type
  uint16_t accent = C_NOTIF;
  const char* icon = "i";
  if (strcmp(n.type, "email") == 0)    { accent = C_NOTIF; icon = "@"; }
  if (strcmp(n.type, "teams") == 0)    { accent = C_TEAMS; icon = "T"; }
  if (strcmp(n.type, "calendar") == 0) { accent = C_STATUS; icon = "C"; }
  if (strcmp(n.type, "urgent") == 0)   { accent = C_WARN;  icon = "!"; }

  // Card background
  tft.fillRoundRect(4, cardY, SW - 8, cardH, 4, 0x2104);
  // Accent bar on left
  tft.fillRect(4, cardY, 4, cardH, accent);

  // Icon badge
  tft.fillCircle(18, cardY + 14, 8, accent);
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE);
  tft.setCursor(15, cardY + 11);
  tft.print(icon);

  // Title
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE);
  tft.setCursor(30, cardY + 8);
  // Truncate title for screen width
  char titleBuf[32];
  strncpy(titleBuf, n.title, min((int)sizeof(titleBuf) - 1, (SW - 40) / 6));
  titleBuf[min((int)sizeof(titleBuf) - 1, (SW - 40) / 6)] = '\0';
  tft.print(titleBuf);

  // Time (top right)
  if (strlen(n.timeStr) > 0) {
    tft.setTextColor(C_LGRAY);
    tft.setCursor(SW - 40, cardY + 8);
    tft.print(n.timeStr);
  }

  // Body (word-wrapped, 2 lines max)
  tft.setTextColor(C_GRAY);
  tft.setCursor(12, cardY + 26);
  int maxChars = (SW - 20) / 6;
  if ((int)strlen(n.body) <= maxChars) {
    tft.print(n.body);
  } else {
    // Line 1
    char line1[48];
    strncpy(line1, n.body, min((int)sizeof(line1) - 1, maxChars));
    line1[min((int)sizeof(line1) - 1, maxChars)] = '\0';
    tft.print(line1);
    // Line 2
    tft.setCursor(12, cardY + 38);
    char line2[48];
    strncpy(line2, n.body + maxChars, min((int)sizeof(line2) - 1, maxChars));
    line2[min((int)sizeof(line2) - 1, maxChars)] = '\0';
    tft.print(line2);
  }

  // Notification count indicator
  if (notifCount > 1) {
    tft.setTextColor(C_LGRAY);
    tft.setCursor(SW / 2 - 10, cardY + cardH - 12);
    char countBuf[10];
    snprintf(countBuf, sizeof(countBuf), "%d/%d", idx + 1, notifCount);
    tft.print(countBuf);
  }

  Serial.printf("[Notif] Showing: [%s] %s\n", n.type, n.title);
}

void dismissNotification() {
  if (!showingNotif) return;

  // Clear notification card area
  int cardY = CLOCK_Y + (CLOCK_SIZE * 8) + 20;
  tft.fillRect(0, cardY, SW, 80, C_BG);

  // Clear scenario card
  clearNotificationV2();
  currentScenario = SC_IDLE;

  // Restore status
  drawStatus(wifiConnected ? "online" : "offline", wifiConnected ? C_STATUS : C_GRAY);

  showingNotif = false;
  L.mood = IDLE;

  // Show next notification if queued
  currentNotif++;
  if (currentNotif < notifCount) {
    showNotification(currentNotif);
    L.mood = ALERT;
  } else {
    notifCount = 0;
    currentNotif = 0;
  }
}

