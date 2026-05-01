/*
 * ╔═════════════════════════════════════════╗
 * ║  RED DEVICE — ClawMagotchi v0.1         ║
 * ║  Digital Clock + Red the Lobster 🦞     ║
 * ║  ESP32-C6 + ILI9341 (Wokwi Emulator)   ║
 * ╚═════════════════════════════════════════╝
 *
 * Idle screen: a digital clock with an animated
 * lobster companion roaming the bottom of the
 * display. He sits, walks, looks around, and
 * occasionally blinks or clicks his claws.
 *
 * Target hardware: ESP32-C6 + ST7789 1.47" LCD
 * Emulator:        Wokwi with ILI9341 (240x320)
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ────────────────────────────────────────────
//  PIN CONFIG (ESP32-C6 defaults: SCK=6, MOSI=7)
// ────────────────────────────────────────────
#define TFT_CS  10
#define TFT_DC  8

// ────────────────────────────────────────────
//  SCREEN
// ────────────────────────────────────────────
#define SW 240
#define SH 320

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

Adafruit_ILI9341 tft(TFT_CS, TFT_DC);

// ════════════════════════════════════════════
//  CLOCK STATE
// ════════════════════════════════════════════
unsigned long tBase;
long simSec;
int prevH = -1, prevM = -1, prevS = -1;
bool colonVis = true;

// ════════════════════════════════════════════
//  LOBSTER STATE MACHINE
// ════════════════════════════════════════════
enum LState { SIT, GO_R, GO_L, LOOK };

struct {
  float x;
  int y;
  LState st;
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

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(C_BG);

  // Dotted ground line
  drawGroundFull();

  // Clock (start at 12:00:00 for demo)
  tBase = millis();
  simSec = 12L * 3600;
  renderTime(12, 0, true);
  renderSec(0);
  renderDate();

  // Status indicator (small teal dot — "connected")
  tft.fillCircle(SW / 2, 185, 3, C_STATUS);
  tft.setTextSize(1);
  tft.setTextColor(C_GROUND);
  tft.setCursor(SW / 2 - 15, 193);
  tft.print("online");

  // Version label
  tft.setTextSize(1);
  tft.setTextColor(C_GROUND);
  tft.setCursor(SW - 60, SH - 10);
  tft.print("Red v0.1");

  // Lobster init
  L.x = SW / 2;
  L.y = LOB_Y;
  L.st = SIT;
  L.fr = 0;
  L.dir = 1;
  L.nextSt = millis() + random(3000, 7000);
  L.nextFr = millis();

  Serial.println("[Red Device] ClawMagotchi v0.1 booted!");
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
}

// ════════════════════════════════════════════
//  CLOCK RENDERING
// ════════════════════════════════════════════

void tickClock(unsigned long now) {
  long t = (simSec + (now - tBase) / 1000) % 86400;
  int h = t / 3600;
  int m = (t % 3600) / 60;
  int s = t % 60;

  colonVis = !colonVis;

  if (h != prevH || m != prevM) {
    renderTime(h, m, colonVis);
    prevH = h;
    prevM = m;
  } else {
    renderColon(colonVis);
  }

  if (s != prevS) {
    renderSec(s);
    prevS = s;
  }
}

void renderTime(int h, int m, bool col) {
  const int y = 55;
  const int cw = 36;  // char width at textSize(6)
  int x0 = (SW - cw * 5) / 2;

  tft.setTextSize(6);
  tft.setTextColor(C_WHITE, C_BG);

  // Hours
  tft.setCursor(x0, y);
  if (h < 10) tft.print('0');
  tft.print(h);

  // Colon
  renderColon(col);

  // Minutes
  tft.setCursor(x0 + cw * 3, y);
  if (m < 10) tft.print('0');
  tft.print(m);
}

void renderColon(bool on) {
  const int y = 55;
  const int cw = 36;
  int x0 = (SW - cw * 5) / 2;

  tft.setTextSize(6);
  tft.setTextColor(on ? C_WHITE : C_DIMWH, C_BG);
  tft.setCursor(x0 + cw * 2, y);
  tft.print(':');
}

void renderSec(int s) {
  int x = (SW - 24) / 2;
  tft.setTextSize(2);
  tft.setTextColor(C_LGRAY, C_BG);
  tft.setCursor(x, 112);
  if (s < 10) tft.print('0');
  tft.print(s);
}

void renderDate() {
  const char* d = "Thu, May 1";
  int w = strlen(d) * 12;
  tft.setTextSize(2);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setCursor((SW - w) / 2, 140);
  tft.print(d);
}

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
