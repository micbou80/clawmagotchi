#pragma once
/*
 * ╔═════════════════════════════════════════╗
 * ║  segments.h — 7-Segment Clock Renderer  ║
 * ║  Procedural rendering via fillRect()    ║
 * ╚═════════════════════════════════════════╝
 */

#include "config.h"
#include <Adafruit_GFX.h>

#ifdef PLATFORM_WAVESHARE
  extern Adafruit_ST7789 tft;
#else
  extern Adafruit_ILI9341 tft;
#endif

// ────────────────────────────────────────────
//  Colors
// ────────────────────────────────────────────
#define SEG_ON   0xFFFF   // white
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
  #define SEG_DW    30    // bigger for readability
  #define SEG_DH    50
  #define SEG_TH    5
  #define SEG_GAP   4
  #define SEG_Y0    30
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
  int w = strlen(dateStr) * 12; // size 2 char = 12px wide
  int x = (SW - w) / 2;

  tft.fillRect(0, dateY - 1, SW, 20, SEG_BG);
  tft.setTextSize(2);
  tft.setTextColor(0x7BEF, SEG_BG); // C_GRAY
  tft.setCursor(x, dateY);
  tft.print(dateStr);
}
