#pragma once
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

#include "config.h"
#include <Adafruit_GFX.h>

#ifdef PLATFORM_WAVESHARE
  extern Adafruit_ST7789 tft;
#else
  extern Adafruit_ILI9341 tft;
#endif

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
enum LobsterExpr {
  EXPR_NORMAL,     // round cyan eyes, neutral
  EXPR_HAPPY,      // squint/arc eyes, small mouth
  EXPR_ALERT,      // wide eyes, raised brows
  EXPR_SLEEPY,     // half-closed eyes
  EXPR_DIZZY,      // spiral/offset eyes
};

// ────────────────────────────────────────────
//  Claw pose enum
// ────────────────────────────────────────────
enum ClawPose {
  CLAW_IDLE,       // relaxed, slightly open
  CLAW_RAISED,     // both up (alert/celebration)
  CLAW_HOLDING,    // one claw holds an item
  CLAW_TUCKED,     // both tucked in (focus/sleep)
  CLAW_TYPING,     // both low and forward (typing on laptop)
};

// ────────────────────────────────────────────
//  Lobster render state (passed to drawLobsterV2)
// ────────────────────────────────────────────
struct LobsterPose {
  int cx, cy;           // center position
  int frame;            // animation frame counter
  int dir;              // facing direction: 1=right, -1=left
  LobsterExpr expr;     // facial expression
  ClawPose clawPose;    // claw state
  const uint8_t* heldIcon;  // 16×16 icon held in claw (NULL = none)
  uint16_t heldColor;   // color for held icon
  float bobOffset;      // vertical bob for idle animation
  float swayOffset;     // horizontal sway
  bool showLaptop;      // draw laptop in front of lobster
};

// Forward declare the 16×16 icon drawer from sprites.h
extern void drawIcon16(int x, int y, const uint8_t* icon, uint16_t color);

// ────────────────────────────────────────────
//  MAIN DRAW FUNCTION
//  Draws the full lobster at given pose.
//  ~80px wide, ~65px tall from antenna tip to feet.
// ────────────────────────────────────────────

// Forward declarations for claw helpers
inline void _drawClaw(int cx, int cy, int side, int openAmt, int f);
inline void _drawClawHolding(int cx, int cy, int side, const uint8_t* icon, uint16_t color, int f);
inline void _drawClawTucked(int cx, int cy, int side);
inline void drawLaptop(int cx, int cy, int frame);

inline void drawLobsterV2(const LobsterPose& p) {
  int cx = p.cx + (int)p.swayOffset;
  int cy = p.cy + (int)p.bobOffset;
  int d  = p.dir;
  int f  = p.frame;

  // ═══════════════════════════════════════
  //  BODY — clean oval torso
  // ═══════════════════════════════════════
  // Main body: simple wide ellipse
  tft.fillCircle(cx, cy, 22, LB_RED);           // center body
  tft.fillCircle(cx - 8, cy, 18, LB_RED);       // left extension
  tft.fillCircle(cx + 8, cy, 18, LB_RED);       // right extension
  // Belly highlight
  tft.fillCircle(cx, cy - 5, 14, LB_LRED);

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

    case CLAW_TYPING: {
      // Claws low and forward, close together, typing motion
      int typOff = (f % 6 < 3) ? 1 : -1;  // subtle typing bounce
      _drawClawTucked(cx - 10, clawY + 8 + typOff, -1);
      _drawClawTucked(cx + 10, clawY + 8 - typOff,  1);
      break;
    }
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

  // ── Laptop overlay (drawn after body so it appears in front) ──
  if (p.showLaptop) drawLaptop(cx, cy, f);
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

// Laptop: tiny computer with animated code on screen
inline void drawLaptop(int cx, int cy, int frame) {
  int sx = cx - 6;   // screen left
  int sy = cy + 8;   // below lobster body

  // Screen bezel
  tft.fillRoundRect(sx - 2, sy - 2, 16, 14, 2, 0x4208);  // dark gray bezel
  // Screen background
  tft.fillRect(sx, sy, 12, 10, LB_BLACK);

  // Animated "code" — tiny colored dots that change each frame
  for (int row = 0; row < 4; row++) {
    int lineLen = 3 + (frame + row) % 5;
    for (int col = 0; col < lineLen && col < 10; col++) {
      uint16_t c;
      if (col == 0) c = 0x07E0;                          // green (prompt)
      else if ((frame + col + row) % 3 == 0) c = 0x07FF; // cyan
      else c = LB_WHITE;
      tft.drawPixel(sx + 1 + col, sy + 1 + row * 2, c);
    }
  }

  // Keyboard/base (wider rectangle below screen)
  int by = sy + 12;
  tft.fillRect(sx - 3, by, 18, 3, 0x6B4D);    // silver/gray base
  tft.drawFastHLine(sx - 3, by, 18, 0x8410);   // highlight line
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
  bottom = p.showLaptop ? cy + 35 : cy + 30;   // laptop extends lower
}
