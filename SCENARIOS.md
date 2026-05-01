# 🎬 ClawMagotchi Scenarios

Complete documentation of the 23-scenario state machine that drives Red the lobster's behavior.

---

## Overview

The scenario system maps real-world situations (detected by Clawpilot) into specific lobster animations, icons, messages, and moods. Each scenario is a self-contained configuration in `scenarios.h`.

### How It Works

1. **Clawpilot** detects a situation (e.g., meeting in 5 minutes)
2. **Bridge** receives a POST with the scenario type
3. **ESP32** polls the bridge, maps the notification to a `Scenario` enum value
4. **Renderer** calls `renderScenarioCard(scenario)` to show the card
5. **Lobster** adjusts mood/animation based on `ScenarioConfig.lobsterMood`
6. After 8 seconds, the card auto-dismisses (or user sends `dismiss`)

---

## All 23 Scenarios

### 1. SC_IDLE — Idle / No News

| Property | Value |
|----------|-------|
| **Emoji** | 😐 |
| **Trigger** | No notifications in queue |
| **Message** | *(none — no card shown)* |
| **Icon** | None |
| **Accent** | Gray (`0x7BEF`) |
| **Mood** | IDLE |
| **Animation** | Slow pacing left/right, random sits/looks |
| **Holds Item** | No |

This is the default state. Red just vibes.

---

### 2. SC_FOCUS — Deep Focus

| Property | Value |
|----------|-------|
| **Emoji** | 🛡️ |
| **Trigger** | Clawpilot detects deep focus (no context switches for 25+ min) |
| **Message** | "Focus" |
| **Icon** | Shield (`ICON_SHIELD`) |
| **Accent** | Green (`0x07E0`) |
| **Mood** | IDLE |
| **Animation** | Standing still, calm |
| **Holds Item** | No |

Signals that you're in flow state. Clawpilot suppresses non-urgent notifications during this.

---

### 3. SC_CONTEXT_SWITCH — Context Switching Too Fast

| Property | Value |
|----------|-------|
| **Emoji** | 😵 |
| **Trigger** | OS-level: 5+ app switches in 2 minutes *(requires desktop agent)* |
| **Message** | "Too fast" |
| **Icon** | Warning (`ICON_WARNING`) |
| **Accent** | Orange (`0xFD20`) |
| **Mood** | ALERT |
| **Animation** | Dizzy wobble, swaying |
| **Holds Item** | No |

⚠️ **Requires OS-level access** — not currently implementable via M365 alone.

---

### 4. SC_TASK_NUDGE — Task Nudge

| Property | Value |
|----------|-------|
| **Emoji** | 👉 |
| **Trigger** | Clawpilot has a task suggestion based on priorities |
| **Message** | "This?" |
| **Icon** | Question (`ICON_QUESTION`) |
| **Accent** | Gold (`0xFE8C`) |
| **Mood** | IDLE |
| **Animation** | Poking forward |
| **Holds Item** | Yes — holds ❓ in claw |

Gentle nudge toward the most impactful task.

---

### 5. SC_LONG_SESSION — Long Session

| Property | Value |
|----------|-------|
| **Emoji** | 😮‍💨 |
| **Trigger** | 90+ minutes without a break |
| **Message** | "Break?" |
| **Icon** | Clock (`ICON_CLOCK`) |
| **Accent** | Orange (`0xFD20`) |
| **Mood** | ALERT |
| **Animation** | Slouching down |
| **Holds Item** | No |

---

### 6. SC_MEETING_SOON — Meeting Soon

| Property | Value |
|----------|-------|
| **Emoji** | 📅 |
| **Trigger** | Calendar event starting in ≤5 minutes |
| **Message** | "5 min" |
| **Icon** | Clock (`ICON_CLOCK`) |
| **Accent** | Blue (`0x2C9F`) |
| **Mood** | ALERT |
| **Animation** | Alert stance, antennae up |
| **Holds Item** | No |

---

### 7. SC_LATE — Late!

| Property | Value |
|----------|-------|
| **Emoji** | 🏃 |
| **Trigger** | Meeting started 1+ minute ago, you haven't joined |
| **Message** | "Late!" |
| **Icon** | Warning (`ICON_WARNING`) |
| **Accent** | Red (`0xE1C8`) |
| **Mood** | URGENT |
| **Animation** | Fast running back and forth |
| **Holds Item** | No |

---

### 8. SC_EMAIL — Important Email

| Property | Value |
|----------|-------|
| **Emoji** | ✉️ |
| **Trigger** | High-priority or VIP email received |
| **Message** | "Mail!" |
| **Icon** | Envelope (`ICON_ENVELOPE`) |
| **Accent** | Blue (`0x2C9F`) |
| **Mood** | ALERT |
| **Animation** | Holds envelope, looks at you |
| **Holds Item** | Yes — holds ✉️ in claw |

---

### 9. SC_TEAMS_MSG — Important Teams Message

| Property | Value |
|----------|-------|
| **Emoji** | 💬 |
| **Trigger** | Direct message or @mention in important channel |
| **Message** | "Ping!" |
| **Icon** | Chat bubble (`ICON_CHAT`) |
| **Accent** | Purple (`0x541F`) |
| **Mood** | ALERT |
| **Animation** | Vibrating, holding chat bubble |
| **Holds Item** | Yes — holds 💬 in claw |

---

### 10. SC_WORTH_REPLY — Worth Replying

| Property | Value |
|----------|-------|
| **Emoji** | ↩️ |
| **Trigger** | Unanswered message that Clawpilot deems important |
| **Message** | "Reply?" |
| **Icon** | Reply arrow (`ICON_REPLY`) |
| **Accent** | Blue (`0x2C9F`) |
| **Mood** | IDLE |
| **Animation** | Holding reply arrow, gentle bounce |
| **Holds Item** | Yes — holds ↩️ in claw |

---

### 11. SC_QUIET_TOO_LONG — Quiet Too Long

| Property | Value |
|----------|-------|
| **Emoji** | 🔇 |
| **Trigger** | No activity detected for 30+ minutes (unusual) |
| **Message** | "Silent..." |
| **Icon** | Eye (`ICON_EYE`) |
| **Accent** | Gray (`0x7BEF`) |
| **Mood** | IDLE |
| **Animation** | Echo gesture, looking around |
| **Holds Item** | No |

---

### 12. SC_MOVE — Move

| Property | Value |
|----------|-------|
| **Emoji** | 🏃 |
| **Trigger** | 2+ hours seated (timer-based) |
| **Message** | "Move" |
| **Icon** | Runner (`ICON_RUNNER`) |
| **Accent** | Orange (`0xFD20`) |
| **Mood** | ALERT |
| **Animation** | Stretching up |
| **Holds Item** | No |

---

### 13. SC_HYDRATE — Hydrate

| Property | Value |
|----------|-------|
| **Emoji** | 💧 |
| **Trigger** | Periodic hydration reminder (every 90 min) |
| **Message** | "Water" |
| **Icon** | Droplet (`ICON_DROPLET`) |
| **Accent** | Gold (`0xFE8C`) |
| **Mood** | IDLE |
| **Animation** | Holding water droplet, offering |
| **Holds Item** | Yes — holds 💧 in claw |

---

### 14. SC_MORNING — Morning Start

| Property | Value |
|----------|-------|
| **Emoji** | 🌅 |
| **Trigger** | First interaction of the day (8-10am) |
| **Message** | "Top 1?" |
| **Icon** | Sunrise (`ICON_SUNRISE`) |
| **Accent** | Gold (`0xFE8C`) |
| **Mood** | HAPPY |
| **Animation** | Waking up, stretching, cheerful |
| **Holds Item** | No |

Prompts you to identify your #1 priority for the day.

---

### 15. SC_SHUTDOWN — Shutdown

| Property | Value |
|----------|-------|
| **Emoji** | 🌙 |
| **Trigger** | End of work day detected (after 5:30pm, activity slowing) |
| **Message** | "Done?" |
| **Icon** | Moon (`ICON_MOON`) |
| **Accent** | Gray (`0x7BEF`) |
| **Mood** | IDLE |
| **Animation** | Curling up, getting sleepy |
| **Holds Item** | No |

---

### 16. SC_WORKING — Agent Working

| Property | Value |
|----------|-------|
| **Emoji** | ⚙️ |
| **Trigger** | Clawpilot is actively processing a task |
| **Message** | "Working" |
| **Icon** | Gear (`ICON_GEAR`) |
| **Accent** | Gray (`0x7BEF`) |
| **Mood** | IDLE |
| **Animation** | Typing at desk, busy |
| **Holds Item** | Yes — holds ⚙️ in claw |

---

### 17. SC_DONE — Task Completed

| Property | Value |
|----------|-------|
| **Emoji** | ✅ |
| **Trigger** | Clawpilot finished a task successfully |
| **Message** | "Done" |
| **Icon** | Checkmark (`ICON_CHECK`) |
| **Accent** | Green (`0x07E0`) |
| **Mood** | HAPPY |
| **Animation** | Celebratory bounce |
| **Holds Item** | No |

---

### 18. SC_NEEDS_INPUT — Needs Your Input

| Property | Value |
|----------|-------|
| **Emoji** | 🤔 |
| **Trigger** | Clawpilot needs a decision or clarification from you |
| **Message** | "You?" |
| **Icon** | Question (`ICON_QUESTION`) |
| **Accent** | Gold (`0xFE8C`) |
| **Mood** | ALERT |
| **Animation** | Head tilt, looking at you |
| **Holds Item** | Yes — holds ❓ in claw |

---

### 19. SC_STUCK — Stuck / Idle Drift

| Property | Value |
|----------|-------|
| **Emoji** | 😕 |
| **Trigger** | Clawpilot detects you haven't made progress (no commits, no docs) |
| **Message** | "Stuck?" |
| **Icon** | Warning (`ICON_WARNING`) |
| **Accent** | Orange (`0xFD20`) |
| **Mood** | ALERT |
| **Animation** | Scratching head |
| **Holds Item** | No |

---

### 20. SC_DECISION — Decision A/B

| Property | Value |
|----------|-------|
| **Emoji** | ⚖️ |
| **Trigger** | Clawpilot identifies a pending decision with clear options |
| **Message** | "A / B" |
| **Icon** | Scales (`ICON_SCALES`) |
| **Accent** | Gold (`0xFE8C`) |
| **Mood** | IDLE |
| **Animation** | Holding two items, weighing |
| **Holds Item** | Yes — holds ⚖️ in claw |

---

### 21. SC_DOOMSCROLL — Doomscrolling

| Property | Value |
|----------|-------|
| **Emoji** | 👀 |
| **Trigger** | OS-level: prolonged passive browsing detected *(requires desktop agent)* |
| **Message** | "Hmm..." |
| **Icon** | Eye (`ICON_EYE`) |
| **Accent** | Gray (`0x7BEF`) |
| **Mood** | IDLE |
| **Animation** | Slow side-eye, judging |
| **Holds Item** | No |

⚠️ **Requires OS-level access** — not currently implementable via M365 alone.

---

### 22. SC_CELEBRATION — Celebration

| Property | Value |
|----------|-------|
| **Emoji** | 🎉 |
| **Trigger** | PR merged, inbox zero, milestone hit |
| **Message** | "Nice." |
| **Icon** | Star (`ICON_STAR`) |
| **Accent** | Green (`0x07E0`) |
| **Mood** | HAPPY |
| **Animation** | Tiny dance, happy bounce |
| **Holds Item** | No |

---

### 23. SC_IDLE_PERSONALITY — Idle Personality

| Property | Value |
|----------|-------|
| **Emoji** | 🤹 |
| **Trigger** | Random, when idle for extended period |
| **Message** | *(none — no card shown)* |
| **Icon** | None |
| **Accent** | Gray (`0x7BEF`) |
| **Mood** | IDLE |
| **Animation** | Juggling, blinking, playful antics |
| **Holds Item** | No |

Just Red being Red. Personality expression with no notification purpose.

---

## Implementation Coverage

### Currently Implementable via Clawpilot (21/23)

These scenarios can be triggered by Clawpilot monitoring M365 signals:

| Scenario | M365 Signal |
|----------|------------|
| Focus | No calendar events, low email rate |
| Task Nudge | Priority analysis from email/tasks |
| Long Session | Timer since last meeting/break |
| Meeting Soon | Calendar API (event in ≤5 min) |
| Late | Calendar + no Teams meeting join |
| Email | Mail API (VIP/high-priority) |
| Teams Message | Chat API (direct message/@mention) |
| Worth Reply | Unanswered thread analysis |
| Quiet Too Long | Absence of M365 activity signals |
| Move | Timer-based |
| Hydrate | Timer-based |
| Morning | First activity + time of day |
| Shutdown | Time of day + declining activity |
| Working | Clawpilot agent state |
| Done | Clawpilot agent state |
| Needs Input | Clawpilot agent state |
| Stuck | Activity pattern analysis |
| Decision | Clawpilot decision detection |
| Celebration | Event detection (PR merge, etc.) |
| Idle Personality | Timer/random |
| Idle | Default state |

### Require OS-Level Access (2/23)

| Scenario | Why |
|----------|-----|
| Context Switching | Needs window/app switch tracking |
| Doomscrolling | Needs browser activity monitoring |

These would require a local desktop agent (e.g., Electron app, AutoHotKey script, or OS accessibility API) to detect app switching frequency and passive browsing patterns.

---

## Triggering Scenarios via Bridge API

### JSON Format

```json
{
  "type": "scenario",
  "scenario": "focus",
  "title": "Optional title override",
  "body": "Optional body text"
}
```

Or using the existing notification types (which map to scenarios):

```json
{"type": "email", "title": "Chris Luce", "body": "Budget review ready"}
```

### Scenario Name Mapping

| Bridge `scenario` value | Enum | Scenario |
|------------------------|------|----------|
| `idle` | SC_IDLE | Idle |
| `focus` | SC_FOCUS | Deep Focus |
| `context_switch` | SC_CONTEXT_SWITCH | Context Switching |
| `task_nudge` | SC_TASK_NUDGE | Task Nudge |
| `long_session` | SC_LONG_SESSION | Long Session |
| `meeting_soon` | SC_MEETING_SOON | Meeting Soon |
| `late` | SC_LATE | Late |
| `email` | SC_EMAIL | Important Email |
| `teams` | SC_TEAMS_MSG | Teams Message |
| `worth_reply` | SC_WORTH_REPLY | Worth Replying |
| `quiet` | SC_QUIET_TOO_LONG | Quiet Too Long |
| `move` | SC_MOVE | Move |
| `hydrate` | SC_HYDRATE | Hydrate |
| `morning` | SC_MORNING | Morning Start |
| `shutdown` | SC_SHUTDOWN | Shutdown |
| `working` | SC_WORKING | Agent Working |
| `done` | SC_DONE | Task Done |
| `needs_input` | SC_NEEDS_INPUT | Needs Input |
| `stuck` | SC_STUCK | Stuck |
| `decision` | SC_DECISION | Decision |
| `doomscroll` | SC_DOOMSCROLL | Doomscrolling |
| `celebration` | SC_CELEBRATION | Celebration |
| `personality` | SC_IDLE_PERSONALITY | Idle Personality |

### curl Examples

```bash
# Deep focus mode
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"focus"}'

# Meeting in 5 minutes
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"calendar","title":"1:1 with Albert","body":"Starting in 5 min"}'

# Agent finished a task
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"done"}'

# Urgent — you're late!
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"urgent","title":"Weekly sync","body":"Started 2 min ago!"}'

# Celebration!
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"celebration"}'

# Hydration reminder
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"hydrate"}'
```

---

## Adding New Scenarios — Step by Step

### Step 1: Add the Enum Value

In `scenarios.h`, add your new scenario before `SC_COUNT`:

```c
enum Scenario {
  // ... existing scenarios ...
  SC_DOOMSCROLL,
  SC_CELEBRATION,
  SC_IDLE_PERSONALITY,
  SC_MY_NEW_SCENARIO,     // ← add here
  SC_COUNT
};
```

### Step 2: Create an Icon (Optional)

If your scenario needs a new icon, create a 16×16 1-bit bitmap in `sprites.h`. See [SPRITES.md](SPRITES.md) for how to design and encode a new icon.

### Step 3: Add the Config Entry

Add a new entry to `SCENARIO_TABLE[]` in `scenarios.h`. The position must match the enum order:

```c
static const ScenarioConfig SCENARIO_TABLE[] = {
  // ... existing entries (must match enum order!) ...

  // SC_MY_NEW_SCENARIO
  { "Hello!",      ICON_STAR,     C_GREEN, C_GREEN, HAPPY,  true,  ICON_STAR },
  //  ↑ message    ↑ card icon    ↑ icon   ↑ accent ↑ mood  ↑ hold ↑ held icon
};
```

### Step 4: Map the Trigger

In `sketch.ino`, add logic to activate your scenario. This could be:
- A new notification `type` from the bridge
- A serial command
- A timer-based trigger
- A specific condition

### Step 5: Test

1. Flash the firmware (or update Wokwi)
2. Send a test notification via serial or bridge:
   ```json
   {"type":"scenario","scenario":"my_new_scenario"}
   ```
3. Verify the card appears with correct icon, message, and color
4. Verify the lobster mood changes appropriately

---

## Mood Behavior Reference

| Mood | Lobster Speed | Animation Style | Trigger Scenarios |
|------|--------------|-----------------|-------------------|
| IDLE | Normal (0.8 px/frame) | Calm pacing, random sits | Idle, Focus, Task Nudge, Worth Reply, Hydrate, Shutdown, Working, Decision, Doomscroll, Personality |
| HAPPY | Faster (1.2 px/frame) | Bouncy, celebratory | Morning, Done, Celebration |
| ALERT | Fast (1.5 px/frame) | Quick movement, looks at card | Context Switch, Long Session, Meeting, Email, Teams, Move, Needs Input, Stuck |
| URGENT | Very fast (2.0 px/frame) | Agitated, red glow | Late |

---

*Scenario docs maintained by Red 🦞*
