#pragma once
/*
 * ╔═════════════════════════════════════════╗
 * ║  scenarios.h — Scenario State Machine   ║
 * ║  23 states → mood, icon, message, color ║
 * ╚═════════════════════════════════════════╝
 */

#include "config.h"
#include "sprites.h"
#include <Adafruit_GFX.h>

#ifdef PLATFORM_WAVESHARE
  extern Adafruit_ST7789 tft;
#else
  extern Adafruit_ILI9341 tft;
#endif

// ────────────────────────────────────────────
//  Color palette (RGB565) for scenario accents
// ────────────────────────────────────────────
#ifndef C_BG
  #define C_BG     0x18C5
#endif
#ifndef C_WARN
  #define C_WARN   0xFD20
#endif
#ifndef C_NOTIF
  #define C_NOTIF  0x2C9F
#endif
#ifndef C_TEAMS
  #define C_TEAMS  0x541F
#endif
#ifndef C_RED
  #define C_RED    0xE1C8
#endif
#ifndef C_GRAY
  #define C_GRAY   0x7BEF
#endif
#ifndef C_GOLD
  #define C_GOLD   0xFE8C
#endif
#define C_GREEN  0x07E0   // positive/done

// ────────────────────────────────────────────
//  Mood enum (must match sketch.ino)
// ────────────────────────────────────────────
// Re-declared here so scenarios.h can be used standalone.
// If LMood is already defined, the compiler will use the
// existing definition (Arduino compiles all into one TU).
#ifndef _LMOOD_DEFINED
#define _LMOOD_DEFINED
enum LMood { IDLE, HAPPY, ALERT, URGENT };
#endif

// ────────────────────────────────────────────
//  Scenario Enum — 23 states
// ────────────────────────────────────────────
enum Scenario {
  SC_IDLE = 0,
  SC_FOCUS,
  SC_CONTEXT_SWITCH,
  SC_TASK_NUDGE,
  SC_LONG_SESSION,
  SC_MEETING_SOON,
  SC_LATE,
  SC_EMAIL,
  SC_TEAMS_MSG,
  SC_WORTH_REPLY,
  SC_QUIET_TOO_LONG,
  SC_MOVE,
  SC_HYDRATE,
  SC_MORNING,
  SC_SHUTDOWN,
  SC_WORKING,
  SC_DONE,
  SC_NEEDS_INPUT,
  SC_STUCK,
  SC_DECISION,
  SC_DOOMSCROLL,
  SC_CELEBRATION,
  SC_IDLE_PERSONALITY,
  SC_COUNT  // sentinel
};

// ────────────────────────────────────────────
//  Scenario Config struct
// ────────────────────────────────────────────
struct ScenarioConfig {
  const char* message;        // short display text (NULL = none)
  const uint8_t* icon;        // PROGMEM icon bitmap (NULL = none)
  uint16_t iconColor;         // icon tint color
  uint16_t accentColor;       // card accent bar color
  LMood lobsterMood;          // lobster mood state
  bool hasHeldItem;           // lobster holds item in claw?
  const uint8_t* heldIcon;    // which icon lobster holds (NULL = none)
};

// ────────────────────────────────────────────
//  Scenario lookup table (PROGMEM-friendly)
//  NOTE: Struct contains pointers so we store
//  in regular memory (still const/read-only).
// ────────────────────────────────────────────
static const ScenarioConfig SCENARIO_TABLE[] = {
  // SC_IDLE
  { NULL,          NULL,           0x0000, C_GRAY,  IDLE,   false, NULL },
  // SC_FOCUS
  { "Focus",      ICON_SHIELD,    0x07E0, C_GREEN, IDLE,   false, NULL },
  // SC_CONTEXT_SWITCH
  { "Too fast",   ICON_WARNING,   C_WARN, C_WARN,  ALERT,  false, NULL },
  // SC_TASK_NUDGE
  { "This?",      ICON_QUESTION,  C_GOLD, C_GOLD,  IDLE,   true,  ICON_QUESTION },
  // SC_LONG_SESSION
  { "Break?",     ICON_CLOCK,     C_WARN, C_WARN,  ALERT,  false, NULL },
  // SC_MEETING_SOON
  { "5 min",      ICON_CLOCK,     C_NOTIF,C_NOTIF, ALERT,  false, NULL },
  // SC_LATE
  { "Late!",      ICON_WARNING,   C_RED,  C_RED,   URGENT, false, NULL },
  // SC_EMAIL
  { "Mail!",      ICON_ENVELOPE,  C_NOTIF,C_NOTIF, ALERT,  true,  ICON_ENVELOPE },
  // SC_TEAMS_MSG
  { "Ping!",      ICON_CHAT,      C_TEAMS,C_TEAMS, ALERT,  true,  ICON_CHAT },
  // SC_WORTH_REPLY
  { "Reply?",     ICON_REPLY,     C_NOTIF,C_NOTIF, IDLE,   true,  ICON_REPLY },
  // SC_QUIET_TOO_LONG
  { "Silent...",  ICON_EYE,       C_GRAY, C_GRAY,  IDLE,   false, NULL },
  // SC_MOVE
  { "Move",       ICON_RUNNER,    C_WARN, C_WARN,  ALERT,  false, NULL },
  // SC_HYDRATE
  { "Water",      ICON_DROPLET,   C_NOTIF,C_GOLD,  IDLE,   true,  ICON_DROPLET },
  // SC_MORNING
  { "Top 1?",     ICON_SUNRISE,   C_GOLD, C_GOLD,  HAPPY,  false, NULL },
  // SC_SHUTDOWN
  { "Done?",      ICON_MOON,      C_GRAY, C_GRAY,  IDLE,   false, NULL },
  // SC_WORKING
  { "Working",    ICON_GEAR,      C_GRAY, C_GRAY,  IDLE,   true,  ICON_GEAR },
  // SC_DONE
  { "Done",       ICON_CHECK,     C_GREEN,C_GREEN, HAPPY,  false, NULL },
  // SC_NEEDS_INPUT
  { "You?",       ICON_QUESTION,  C_GOLD, C_GOLD,  ALERT,  true,  ICON_QUESTION },
  // SC_STUCK
  { "Stuck?",     ICON_WARNING,   C_WARN, C_WARN,  ALERT,  false, NULL },
  // SC_DECISION
  { "A / B",      ICON_SCALES,    C_GOLD, C_GOLD,  IDLE,   true,  ICON_SCALES },
  // SC_DOOMSCROLL
  { "Hmm...",     ICON_EYE,       C_GRAY, C_GRAY,  IDLE,   false, NULL },
  // SC_CELEBRATION
  { "Nice.",      ICON_STAR,      C_GREEN,C_GREEN, HAPPY,  false, NULL },
  // SC_IDLE_PERSONALITY
  { NULL,          NULL,           0x0000, C_GRAY,  IDLE,   false, NULL },
};

// ────────────────────────────────────────────
//  Public API
// ────────────────────────────────────────────

inline const ScenarioConfig& getScenario(Scenario sc) {
  if (sc >= SC_COUNT) sc = SC_IDLE;
  return SCENARIO_TABLE[sc];
}

// Card layout constants
#define CARD_Y_OFFSET  90   // below clock+date area
#define CARD_H         36
#define CARD_MARGIN    6
#define CARD_RADIUS    4

/*
 * Render a notification-style card for the active scenario.
 * Positioned below the clock/date region.
 */
inline void renderScenarioCard(Scenario sc) {
  const ScenarioConfig& cfg = getScenario(sc);
  if (cfg.message == NULL) return; // no card for idle states

  // Card position
  #if SW >= 200
    int cardY = SEG_Y0 + SEG_DH + 48;
  #else
    int cardY = SEG_Y0 + SEG_DH + 40;
  #endif
  int cardW = SW - 2 * CARD_MARGIN;

  // Dark rounded rect background
  tft.fillRoundRect(CARD_MARGIN, cardY, cardW, CARD_H, CARD_RADIUS, 0x2104);

  // Accent bar on left
  tft.fillRect(CARD_MARGIN, cardY, 3, CARD_H, cfg.accentColor);

  // Icon badge on left
  if (cfg.icon != NULL) {
    int iconX = CARD_MARGIN + 8;
    int iconY = cardY + (CARD_H - 16) / 2;
    drawIconBadge(iconX, iconY, cfg.icon, 0xFFFF, cfg.accentColor);
  }

  // Message text — right of icon
  if (cfg.message != NULL) {
    int textX = CARD_MARGIN + 34;
    int textY = cardY + (CARD_H - 14) / 2;
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF, 0x2104);
    tft.setCursor(textX, textY);
    tft.print(cfg.message);
  }
}

/*
 * Clear the scenario card area
 */
inline void clearScenarioCard() {
  #if SW >= 200
    int cardY = SEG_Y0 + SEG_DH + 48;
  #else
    int cardY = SEG_Y0 + SEG_DH + 40;
  #endif
  tft.fillRect(0, cardY, SW, CARD_H + 4, C_BG);
}
