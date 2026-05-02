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

#include "config.h"
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
#endif

#include "segments.h"
#include "sprites.h"
#include "scenarios.h"
#include "lobster_gfx.h"

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
  int countdown;     // seconds for countdown timer (0 = no countdown)
  bool active;
};
Notification notifs[MAX_NOTIFS];
int notifCount = 0;
bool showingNotif = false;
unsigned long notifShowTime = 0;
int currentNotif = 0;

// ════════════════════════════════════════════
//  COUNTDOWN TIMER STATE
// ════════════════════════════════════════════
bool countdownActive = false;
bool workingActive = false;              // persistent "working" state
unsigned long countdownStart = 0;    // millis() when countdown began
int countdownTotal = 0;              // total seconds to count down
char countdownTitle[64] = "";        // meeting title during countdown
uint16_t countdownAccent = 0;        // accent color

// ════════════════════════════════════════════
//  SCENARIO STATE
// ════════════════════════════════════════════
Scenario currentScenario = SC_IDLE;

// ════════════════════════════════════════════
//  LOBSTER STATE MACHINE
// ════════════════════════════════════════════
enum LState { SIT, GO_R, GO_L, LOOK };
#ifndef _LMOOD_DEFINED
#define _LMOOD_DEFINED
enum LMood  { IDLE, HAPPY, ALERT, URGENT };
#endif

LobsterPose lobPose;
LMood lobMood = IDLE;
float lobXf;  // floating point x for smooth sub-pixel movement
unsigned long lobNextFr = 0;

struct {
  LState st;
  unsigned long nextSt;
} lobSM;

// Bounding box of previous frame (for erase)
int lpL = 0, lpR = 0, lpT = 0, lpB = 0;

// Ground Y position
#define GROUND_Y (SH - 80)
#define LOB_Y    (SH - 40)

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
  seg7_renderDate("Thu, May 1");

  // Version label
  tft.setTextSize(1);
  tft.setTextColor(C_GROUND);
  tft.setCursor(SW - 60, SH - 10);
  tft.print("Red v0.2");

  // Lobster init
  lobXf = SW / 2;
  lobPose.cx = SW / 2;
  lobPose.cy = LOB_Y;
  lobPose.frame = 0;
  lobPose.dir = 1;
  lobPose.expr = EXPR_NORMAL;
  lobPose.clawPose = CLAW_IDLE;
  lobPose.heldIcon = NULL;
  lobPose.heldColor = 0;
  lobPose.bobOffset = 0;
  lobPose.swayOffset = 0;
  lobPose.showLaptop = false;
  lobSM.st = SIT;
  lobSM.nextSt = millis() + random(3000, 7000);
  lobNextFr = millis();

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
  if (now >= lobNextFr) {
    lobNextFr = now + 83;
    tickLobster(now);
  }

  // Notification display timeout (show for 20 sec, then return to clock)
  // BUT not while a countdown timer is active
  if (showingNotif && !countdownActive && !workingActive && (now - notifShowTime > 20000)) {
    dismissNotification();
  }

  // Countdown timer: update every second
  static unsigned long lastCountdownTick = 0;
  if (countdownActive && (now - lastCountdownTick >= 1000)) {
    lastCountdownTick = now;
    renderCountdown();
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
  if (lpR > lpL) {
    tft.fillRect(lpL, lpT, lpR - lpL, lpB - lpT, C_BG);
    // Restore ground dots inside erased area
    if (lpT <= GROUND_Y && lpB >= GROUND_Y) {
      drawGroundRange(lpL, lpR);
    }
  }

  // ── State transitions ──
  if (now >= lobSM.nextSt) {
    int r = random(100);

    if (lobSM.st == LOOK) {
      if (r < 40)      { lobSM.st = SIT;  lobSM.nextSt = now + random(3000, 7000); }
      else if (r < 70) { lobSM.st = GO_R; lobPose.dir = 1;  lobSM.nextSt = now + random(2000, 5000); }
      else             { lobSM.st = GO_L; lobPose.dir = -1; lobSM.nextSt = now + random(2000, 5000); }
    }
    else if (r < 30) {
      lobSM.st = SIT;
      lobSM.nextSt = now + random(3000, 8000);
    }
    else if (r < 50) {
      lobSM.st = GO_R;
      lobPose.dir = 1;
      lobSM.nextSt = now + random(2000, 5000);
    }
    else if (r < 70) {
      lobSM.st = GO_L;
      lobPose.dir = -1;
      lobSM.nextSt = now + random(2000, 5000);
    }
    else {
      lobSM.st = LOOK;
      lobSM.nextSt = now + random(1500, 3000);
    }
  }

  // ── Movement ──
  float speed = 0.6;  // slightly slower for bigger lobster
  if (lobSM.st == GO_R) {
    lobXf += speed;
    if (lobXf > SW - 45) { lobXf = SW - 45; lobSM.st = SIT; lobSM.nextSt = now + 2000; }
  }
  else if (lobSM.st == GO_L) {
    lobXf -= speed;
    if (lobXf < 45) { lobXf = 45; lobSM.st = SIT; lobSM.nextSt = now + 2000; }
  }
  else if (lobSM.st == LOOK) {
    if (lobPose.frame % 10 == 0) lobPose.dir = -lobPose.dir;
  }

  lobPose.cx = (int)lobXf;

  // ── Idle bob animation ──
  lobPose.bobOffset = sin(lobPose.frame * 0.08) * 2.0;
  lobPose.swayOffset = sin(lobPose.frame * 0.03) * 1.0;

  // ── Map mood to expression & claw pose ──
  // Don't override if working state is active (typing pose managed separately)
  if (!workingActive) {
    switch (lobMood) {
      case IDLE:
        lobPose.expr = EXPR_NORMAL;
        lobPose.clawPose = CLAW_IDLE;
        break;
      case HAPPY:
        lobPose.expr = EXPR_HAPPY;
        lobPose.clawPose = CLAW_RAISED;
        break;
      case ALERT:
        lobPose.expr = EXPR_ALERT;
        lobPose.clawPose = CLAW_RAISED;
        break;
      case URGENT:
        lobPose.expr = EXPR_ALERT;
        lobPose.clawPose = CLAW_RAISED;
        break;
    }
  }

  lobPose.frame++;

  // ── Draw ──
  drawLobsterV2(lobPose);

  // ── Store bounds for next erase ──
  getLobsterBounds(lobPose, lpL, lpT, lpR, lpB);
}

// ════════════════════════════════════════════
//  STATUS INDICATOR
// ════════════════════════════════════════════

void drawStatus(const char* label, uint16_t color) {
  int y = CLOCK_Y + (CLOCK_SIZE * 8) + 52;
  tft.fillRect(0, y - 2, SW, 22, C_BG);
  // Large colored dot only, no text label
  tft.fillCircle(SW / 2, y + 5, 5, color);
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
    n.countdown = obj["countdown"] | 0;
    n.active = true;
    notifCount++;
  }

  if (notifCount > 0 && !showingNotif) {
    currentNotif = 0;
    showNotification(currentNotif);

    // Handle working_done: dismiss working state immediately
    if (strcmp(notifs[0].type, "working_done") == 0) {
      workingActive = false;
      dismissNotification();
      return;
    }

    // Set mood based on type (working sets its own mood in showNotification)
    if (strcmp(notifs[0].type, "working") != 0) {
      lobMood = (strcmp(notifs[0].type, "urgent") == 0) ? URGENT : ALERT;
    }
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
      wifiConnected ? "yes" : "no", notifCount, lobMood);
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
  n.countdown = 0;

  // Extract countdown
  ti = json.indexOf("\"countdown\"");
  if (ti >= 0) {
    int ci = json.indexOf(':', ti) + 1;
    while (ci < (int)json.length() && json.charAt(ci) == ' ') ci++;
    n.countdown = json.substring(ci).toInt();
  }

  showNotification(0);

  // Handle working_done: dismiss working state immediately
  if (strcmp(n.type, "working_done") == 0) {
    workingActive = false;
    dismissNotification();
    return;
  }

  // Set mood based on type (working sets its own mood in showNotification)
  if (strcmp(n.type, "working") != 0) {
    lobMood = (strcmp(n.type, "urgent") == 0) ? URGENT : ALERT;
  }
}
#endif

// ════════════════════════════════════════════
//  WORD-WRAP TEXT HELPER
// ════════════════════════════════════════════

// Word-wrap text and render line by line.
// Returns the Y position after the last line drawn.
int drawWrappedText(const char* text, int x, int y, int maxWidth, int textSize, uint16_t color, int maxLines, bool centered = false) {
  tft.setTextSize(textSize);
  tft.setTextColor(color, C_BG);
  int charW = 6 * textSize;
  int lineH = 8 * textSize + 2;
  int maxChars = maxWidth / charW;
  int len = strlen(text);
  int pos = 0;
  int linesDrawn = 0;

  while (pos < len && linesDrawn < maxLines) {
    int lineLen = min(maxChars, len - pos);

    // Try to break at a space if not at end of text
    if (pos + lineLen < len && lineLen == maxChars) {
      int lastSpace = -1;
      for (int i = lineLen - 1; i > 0; i--) {
        if (text[pos + i] == ' ') { lastSpace = i; break; }
      }
      if (lastSpace > 0) lineLen = lastSpace + 1;
    }

    // Count actual printable chars for centering
    int printLen = lineLen;
    while (printLen > 0 && text[pos + printLen - 1] == ' ') printLen--;

    // Draw this line
    int cx = x;
    if (centered) {
      cx = (SW - printLen * charW) / 2;
    }
    tft.setCursor(cx, y);
    for (int i = 0; i < lineLen; i++) {
      char c = text[pos + i];
      if (c != '\n') tft.print(c);
    }

    pos += lineLen;
    while (pos < len && text[pos] == ' ') pos++;
    y += lineH;
    linesDrawn++;
  }

  return y;
}

// ════════════════════════════════════════════
//  COUNTDOWN TIMER RENDERING
// ════════════════════════════════════════════

void renderCountdown() {
  unsigned long elapsed = (millis() - countdownStart) / 1000;
  int remaining = countdownTotal - (int)elapsed;

  if (remaining <= 0) {
    // Timer done — flash effect then dismiss
    countdownActive = false;
    for (int i = 0; i < 3; i++) {
      tft.fillScreen(countdownAccent);
      delay(150);
      tft.fillScreen(C_BG);
      delay(150);
    }
    dismissNotification();
    return;
  }

  int mins = remaining / 60;
  int secs = remaining % 60;

  // Render below the notification title area — inside the notification zone
  int areaTop = SEG_Y0 + SEG_DH + 52;
  int timerY = areaTop + 50;  // below type label + title

  // Clear just the countdown area to avoid flicker
  tft.fillRect(0, timerY, SW, 50, C_BG);

  // Big countdown digits
  char timeBuf[8];
  snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", mins, secs);

  int textSize = 4;
  int charW = 6 * textSize;
  int textW = strlen(timeBuf) * charW;
  int x = (SW - textW) / 2;

  // Color: white normally, red under 60s, flashing orange under 10s
  uint16_t color = C_WHITE;
  if (remaining < 60) color = 0xF800;  // red
  if (remaining < 10 && (remaining % 2 == 0)) color = C_WARN;  // flashing orange

  tft.setTextSize(textSize);
  tft.setTextColor(color, C_BG);
  tft.setCursor(x, timerY + 5);
  tft.print(timeBuf);

  // "until meeting" label below
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_BG);
  int labelW = strlen("until meeting") * 6;
  tft.setCursor((SW - labelW) / 2, timerY + 42);
  tft.print("until meeting");
}

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

  // Pick accent color and icon char based on type
  uint16_t accent = C_NOTIF;
  const char* icon = "i";
  if (strcmp(n.type, "email") == 0)    { accent = C_NOTIF;  icon = "@"; }
  if (strcmp(n.type, "teams") == 0)    { accent = C_TEAMS;  icon = "#"; }
  if (strcmp(n.type, "calendar") == 0) { accent = C_STATUS;  icon = "C"; }
  if (strcmp(n.type, "urgent") == 0)   { accent = C_WARN;   icon = "!"; }

  // Full-screen takeover: clear area between clock region and ground
  int areaTop = SEG_Y0 + SEG_DH + 52;   // below date + status dot
  int areaBot = GROUND_Y - 2;
  tft.fillRect(0, areaTop, SW, areaBot - areaTop, C_BG);

  int y = areaTop + 8;
  int margin = 8;
  int textWidth = SW - 2 * margin;  // 156px

  // ── Title (size 2, white, centered, max 3 lines) ──
  y = drawWrappedText(n.title, margin, y, textWidth, 2, C_WHITE, 3, true);
  y += 6;

  // ── Body (size 2, gray, centered, max 4 lines) ──
  if (strlen(n.body) > 0) {
    y = drawWrappedText(n.body, margin, y, textWidth, 2, C_GRAY, 4, true);
  }

  // ── Counter "1/3" if multiple (size 1, centered, dim) ──
  if (notifCount > 1) {
    int counterY = areaBot - 12;
    char countBuf[10];
    snprintf(countBuf, sizeof(countBuf), "%d/%d", idx + 1, notifCount);
    int cw = strlen(countBuf) * 6;
    tft.setTextSize(1);
    tft.setTextColor(C_LGRAY, C_BG);
    tft.setCursor((SW - cw) / 2, counterY);
    tft.print(countBuf);
  }

  Serial.printf("[Notif] Showing: [%s] %s\n", n.type, n.title);

  // Activate countdown timer if present
  if (n.countdown > 0) {
    countdownActive = true;
    countdownStart = millis();
    countdownTotal = n.countdown;
    strlcpy(countdownTitle, n.title, sizeof(countdownTitle));
    countdownAccent = accent;
    Serial.printf("[Countdown] Started: %d seconds\n", n.countdown);
  }

  // Activate working state if type is "working"
  if (strcmp(n.type, "working") == 0) {
    workingActive = true;
    lobMood = IDLE;
    lobPose.expr = EXPR_NORMAL;
    lobPose.clawPose = CLAW_TYPING;
    lobPose.showLaptop = true;
  }
}

void dismissNotification() {
  if (!showingNotif) return;

  // Reset countdown and working state before mood reset
  countdownActive = false;
  workingActive = false;
  lobPose.showLaptop = false;
  lobPose.clawPose = CLAW_IDLE;

  // Clear full notification takeover area (between status and ground)
  int areaTop = SEG_Y0 + SEG_DH + 52;
  int areaBot = GROUND_Y - 2;
  tft.fillRect(0, areaTop, SW, areaBot - areaTop, C_BG);

  // Clear scenario card area too
  clearScenarioCard();
  currentScenario = SC_IDLE;

  // Restore status dot and ground line
  drawStatus(wifiConnected ? "online" : "offline", wifiConnected ? C_STATUS : C_GRAY);
  drawGroundFull();

  showingNotif = false;
  lobMood = IDLE;

  // Show next notification if queued
  currentNotif++;
  if (currentNotif < notifCount) {
    showNotification(currentNotif);
    lobMood = ALERT;
  } else {
    notifCount = 0;
    currentNotif = 0;
  }
}
