#pragma once
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

#include "config.h"
#include <Adafruit_GFX.h>

#ifdef PLATFORM_WAVESHARE
  extern Adafruit_ST7789 tft;
#else
  extern Adafruit_ILI9341 tft;
#endif

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
