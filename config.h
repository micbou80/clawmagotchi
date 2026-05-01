/*
 * ╔═════════════════════════════════════════╗
 * ║  RED DEVICE — Configuration             ║
 * ║  WiFi, API, Hardware Settings           ║
 * ╚═════════════════════════════════════════╝
 */

#pragma once

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
