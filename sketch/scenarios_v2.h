#pragma once
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

#include "config.h"
#include "lobster_gfx.h"
#include "icons_large.h"
#include "sprites.h"   // still used for 16×16 held items

// ────────────────────────────────────────────
//  Scenario → Lobster mapping
// ────────────────────────────────────────────

// Icon draw function pointer type
typedef void (*IconDrawFn)(int cx, int cy);

struct ScenarioV2Config {
  const char* message;        // display text (NULL = none)
  IconDrawFn  drawIcon;       // large icon draw function (NULL = none)
  uint16_t    accentColor;    // text/accent color
  LobsterExpr expression;     // lobster face
  ClawPose    clawPose;       // claw state
  const uint8_t* heldItem;    // 16×16 icon for claw (NULL = none)
  uint16_t    heldColor;      // color for held item
};

// ────────────────────────────────────────────
//  Scenario Enum (re-use from scenarios.h)
// ────────────────────────────────────────────
#ifndef _SCENARIO_V2_ENUM
#define _SCENARIO_V2_ENUM
enum ScenarioV2 {
  S2_IDLE = 0,
  S2_FOCUS,
  S2_CONTEXT_SWITCH,
  S2_TASK_NUDGE,
  S2_LONG_SESSION,
  S2_MEETING_SOON,
  S2_LATE,
  S2_EMAIL,
  S2_TEAMS_MSG,
  S2_WORTH_REPLY,
  S2_QUIET_TOO_LONG,
  S2_MOVE,
  S2_HYDRATE,
  S2_MORNING,
  S2_SHUTDOWN,
  S2_WORKING,
  S2_DONE,
  S2_NEEDS_INPUT,
  S2_STUCK,
  S2_DECISION,
  S2_DOOMSCROLL,
  S2_CELEBRATION,
  S2_IDLE_PERSONALITY,
  S2_COUNT
};
#endif

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
