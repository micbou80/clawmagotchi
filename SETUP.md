# 🦞 Red Device — Hardware Setup Guide

Everything you need to go from unboxing to a working ClawMagotchi on real hardware.

---

## What You Need

- Waveshare ESP32-C6-LCD-1.47 board
- USB-C cable
- Your PC on the same WiFi network as the device

---

## Step 1 — Install Arduino IDE + Board Support

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Go to **File → Preferences**, add this board manager URL:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **Tools → Board → Board Manager** → search "esp32" → install **esp32 by Espressif**
4. Select board: **ESP32C6 Dev Module**

---

## Step 2 — Install Libraries

Via **Sketch → Include Library → Manage Libraries**, install:

- Adafruit GFX Library
- Adafruit ST7789
- ArduinoJson

---

## Step 3 — Configure `config.h`

1. Set your WiFi credentials:
   ```c
   #define WIFI_SSID     "YourNetworkName"
   #define WIFI_PASS     "YourPassword"
   ```

2. Find your PC's local IP:
   ```powershell
   ipconfig
   ```
   Look for the **IPv4 Address** under your WiFi adapter (e.g. `192.168.1.100`).

3. Set the bridge IP:
   ```c
   #define API_HOST      "192.168.1.100"
   ```

4. Switch to the real hardware platform:
   ```c
   #define PLATFORM_WAVESHARE
   // #define PLATFORM_WOKWI
   ```

---

## Step 4 — Flash the Device

1. Open `sketch.ino` in Arduino IDE
2. Connect the board via USB-C
3. Select the COM port under **Tools → Port**
4. Click **Upload** ▶️
5. Open **Tools → Serial Monitor** (baud: 115200) to watch boot messages

---

## Step 5 — Start the Bridge on Your PC

```powershell
cd "C:\Users\micbou\OneDrive - Microsoft\Documents\Clawpilot\red-device\bridge"
python server.py
```

You should see:
```
🦞 Red Device Bridge running on http://0.0.0.0:8222
   Device polls:  GET  /api/notifications
   Push notifs:   POST /api/notifications
   Health check:  GET  /api/status
   Clear queue:   POST /api/clear
```

### Firewall (first time only)

If the ESP32 can't reach the bridge, allow inbound traffic on port 8222:

```powershell
New-NetFirewallRule -DisplayName "Red Device Bridge" -Direction Inbound -LocalPort 8222 -Protocol TCP -Action Allow
```

---

## Step 6 — Test It!

Push a test notification from PowerShell:

```powershell
Invoke-RestMethod -Method Post -Uri "http://localhost:8222/api/notifications" `
  -ContentType "application/json" `
  -Body '{"type":"email","title":"Test","body":"Hello from Clawpilot!"}'
```

The device should pick it up within 5 seconds and display the notification. 🦞

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Bridge not reachable from device | Check `API_HOST` in `config.h` matches your PC's IP |
| Device won't connect to WiFi | Double-check SSID/password, ensure 2.4GHz network |
| Upload fails | Hold BOOT button while clicking Upload, release after "Connecting..." |
| No display output | Verify `PLATFORM_WAVESHARE` is uncommented in `config.h` |
| Notifications not arriving | Run `curl http://localhost:8222/api/status` — is bridge running? |
| Port blocked | Run the firewall rule above |

### Serial Monitor Checklist

When the device boots successfully you should see:
1. ✅ WiFi connected (with IP address)
2. ✅ NTP time synced
3. ✅ First poll to bridge (200 OK or connection refused if bridge isn't running yet)

---

## Running the Bridge Persistently

### Option A — Background PowerShell job

```powershell
Start-Process python -ArgumentList "server.py" `
  -WorkingDirectory "C:\Users\micbou\OneDrive - Microsoft\Documents\Clawpilot\red-device\bridge" `
  -WindowStyle Hidden
```

### Option B — Windows Task Scheduler

1. Open Task Scheduler → Create Basic Task
2. Name: "Red Device Bridge"
3. Trigger: "When I log on"
4. Action: Start a program
   - Program: `python`
   - Arguments: `server.py`
   - Start in: `C:\Users\micbou\OneDrive - Microsoft\Documents\Clawpilot\red-device\bridge`

---

## Quick Reference

| Action | Command |
|--------|---------|
| Start bridge | `python server.py` |
| Check bridge health | `curl http://localhost:8222/api/status` |
| Push notification | `python push.py --type email --title "Test" --body "Hello"` |
| Clear queue | `curl -X POST http://localhost:8222/api/clear` |
| Batch test | `python test_push.py` |

---

*Happy hacking! — Red 🦞*
