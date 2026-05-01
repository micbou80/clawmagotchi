# 🌉 Red Device — Notification Bridge

HTTP server that sits between Clawpilot and the ESP32 ClawMagotchi device. Pure Python 3 stdlib — **zero dependencies**, no pip install needed.

---

## How It Works

```
┌─────────────┐                       ┌─────────────┐                      ┌─────────────┐
│  Clawpilot  │──POST /api/notifs──►  │   Bridge    │  ◄──GET /api/notifs── │   ESP32     │
│  (AI agent) │                       │  Server.py  │                      │  (Red 🦞)   │
└─────────────┘                       └──────┬──────┘                      └─────────────┘
                                             │
                                       Queue in memory
                                       (max 10, FIFO)
```

1. **Clawpilot** pushes notifications via `POST /api/notifications`
2. **Bridge** holds them in a thread-safe queue (max 10)
3. **ESP32** polls `GET /api/notifications` every 5 seconds
4. Each poll returns all queued items and **clears the queue**
5. If queue is full, new pushes are silently dropped

---

## Quick Start

```bash
# Start the bridge (default port 8222)
python server.py

# Custom port
python server.py --port 9000

# Custom host (default is 0.0.0.0 = all interfaces)
python server.py --host 127.0.0.1 --port 8222
```

You should see:
```
🦞 Red Device Bridge running on http://0.0.0.0:8222
   Device polls:  GET  /api/notifications
   Push notifs:   POST /api/notifications
   Health check:  GET  /api/status
   Clear queue:   POST /api/clear
```

---

## API Reference

### `GET /api/notifications` — Device Poll

Returns all queued notifications and **clears the queue**.

**Response:**
```json
{
  "notifications": [
    {
      "type": "email",
      "title": "Chris Luce",
      "body": "FY27 budget review",
      "time": "09:41"
    }
  ]
}
```

Empty queue returns `{"notifications": []}`.

---

### `POST /api/notifications` — Push Notification

Send a single notification or an array of notifications.

**Single notification:**
```json
{
  "type": "email",
  "title": "Chris Luce",
  "body": "Budget review ready",
  "time": "09:41"
}
```

**Array of notifications:**
```json
[
  {"type": "email", "title": "Chris", "body": "Budget review"},
  {"type": "teams", "title": "Team Chat", "body": "Julien shared agenda"}
]
```

**Fields:**

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `type` | No | `"info"` | `email`, `teams`, `calendar`, `urgent`, `info`, `scenario` |
| `title` | No | `"Notification"` | Short title (shown on card) |
| `body` | No | `""` | Body text |
| `time` | No | Current time (`HH:MM`) | Timestamp string |
| `scenario` | No | — | Scenario name (when type=`scenario`) |

**Response:**
```json
{"queued": 1, "total": 3}
```

---

### `GET /api/status` — Health Check

```json
{
  "status": "running",
  "queue_size": 2,
  "max_queue": 10,
  "total_pushed": 15,
  "total_polled": 42,
  "last_push": "2025-06-15T09:41:00",
  "last_poll": "2025-06-15T09:41:05",
  "device_ip": "192.168.1.50",
  "started": "2025-06-15T08:00:00"
}
```

---

### `POST /api/clear` — Clear Queue

Removes all queued notifications without delivering them.

```json
{"cleared": 3}
```

---

## curl Examples

### Basic Notifications

```bash
# Email notification
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"email","title":"Chris Luce","body":"FY27 budget review — need your input by EOD"}'

# Teams message
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"teams","title":"Dream Team","body":"Julien shared the Milan agenda"}'

# Calendar reminder
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"calendar","title":"1:1 with Albert","body":"Starting in 5 minutes"}'

# Urgent alert
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"urgent","title":"Production Alert","body":"Teams Rooms portal returning 503"}'
```

### Scenario Notifications

```bash
# Deep focus mode
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"focus"}'

# Agent working on a task
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"working"}'

# Task completed
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"done"}'

# Needs your input
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"needs_input"}'

# Hydration reminder
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"hydrate"}'

# Morning start
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"morning"}'

# Celebration
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"scenario","scenario":"celebration"}'
```

### Management

```bash
# Check bridge status
curl http://localhost:8222/api/status

# See what's in the queue (also clears it!)
curl http://localhost:8222/api/notifications

# Clear the queue without delivering
curl -X POST http://localhost:8222/api/clear

# Push multiple at once
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '[
    {"type":"email","title":"Chris","body":"Budget"},
    {"type":"teams","title":"Team","body":"Agenda shared"}
  ]'
```

### Windows (PowerShell)

```powershell
# PowerShell uses different quoting
Invoke-RestMethod -Method Post -Uri "http://localhost:8222/api/notifications" `
  -ContentType "application/json" `
  -Body '{"type":"email","title":"Chris Luce","body":"Budget review ready"}'

# Check status
Invoke-RestMethod -Uri "http://localhost:8222/api/status"
```

---

## Python Examples

### Using push.py (CLI)

```bash
# Basic usage
python push.py --type email --title "Chris Luce" --body "Budget review ready"

# Teams message
python push.py --type teams --title "Dream Team" --body "Julien shared agenda"

# Urgent with custom time
python push.py --type urgent --title "Alert" --body "Service down" --time "14:30"

# Custom bridge URL
python push.py --type info --title "Test" --body "Hello" --url http://192.168.1.100:8222/api/notifications
```

### Using push.py (Importable)

```python
from push import push_notification, is_bridge_running

# Check if bridge is available
if is_bridge_running():
    # Push a notification
    success = push_notification("email", "Chris Luce", "Budget review ready")
    
    # Push with custom time
    push_notification("urgent", "Alert", "Service down", time_str="14:30")
    
    # Push to remote bridge
    push_notification("teams", "Chat", "Hello", 
                      bridge_url="http://192.168.1.100:8222/api/notifications")
```

### Batch Testing

```bash
# Push 4 sample notifications (email, teams, calendar, urgent)
python test_push.py
```

---

## Clawpilot Integration

### The `/red-notify` Skill

Clawpilot has a built-in skill called `/red-notify` that pushes scenario-appropriate notifications to the bridge. When Clawpilot detects relevant signals (new email, meeting starting, focus state), it automatically calls:

```python
push_notification(scenario_type, title, body)
```

### Signal → Scenario Mapping

| Clawpilot Signal | Scenario Pushed |
|-----------------|-----------------|
| VIP email received | `email` |
| Direct Teams message | `teams` |
| Meeting in ≤5 min | `meeting_soon` |
| Meeting started, not joined | `late` |
| Agent starts working | `working` |
| Agent completes task | `done` |
| Agent needs decision | `needs_input` |
| No activity for 30 min | `quiet` |
| 90 min without break | `long_session` |
| First morning activity | `morning` |
| Evening wind-down | `shutdown` |
| PR merged / milestone | `celebration` |

### Custom Integration

Any tool that can make HTTP POST requests can push to the bridge:

- **Home Assistant**: HTTP POST action in automations
- **IFTTT**: Webhook action
- **n8n / Make**: HTTP Request node
- **GitHub Actions**: curl in workflow step
- **Cron jobs**: Simple bash script with curl

---

## Configuration on ESP32

Set these in `config.h`:

```c
#define API_HOST      "192.168.1.100"   // IP of the PC running bridge
#define API_PORT      8222              // Must match server.py port
#define API_PATH      "/api/notifications"
#define POLL_INTERVAL 5000             // ms between polls (5 seconds)
```

**Finding your PC's IP:**
- Windows: `ipconfig` → look for IPv4 Address under your WiFi adapter
- Mac: `ifconfig en0` → look for `inet`
- Linux: `ip addr show` → look for `inet` under your interface

---

## Running as a Background Service

### Windows (Task Scheduler)

1. Open Task Scheduler
2. Create Basic Task → name it "Red Device Bridge"
3. Trigger: "When the computer starts" or "When I log on"
4. Action: Start a program
   - Program: `python` (or full path to `python.exe`)
   - Arguments: `server.py`
   - Start in: full path to the `bridge/` folder
5. Check "Run whether user is logged on or not" in Properties

### Windows (PowerShell background)

```powershell
# Start in background
Start-Process python -ArgumentList "server.py" -WorkingDirectory "C:\path\to\bridge" -WindowStyle Hidden

# Or with nohup-style approach
Start-Job { Set-Location "C:\path\to\bridge"; python server.py }
```

### Linux/Mac (systemd)

Create `/etc/systemd/system/red-bridge.service`:

```ini
[Unit]
Description=Red Device Notification Bridge
After=network.target

[Service]
Type=simple
User=your-username
WorkingDirectory=/path/to/red-device/bridge
ExecStart=/usr/bin/python3 server.py
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Then:
```bash
sudo systemctl enable red-bridge
sudo systemctl start red-bridge
sudo systemctl status red-bridge
```

### Linux/Mac (simple)

```bash
# Background with nohup
nohup python3 server.py &

# With output logging
nohup python3 server.py > bridge.log 2>&1 &
```

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `Connection refused` when pushing | Bridge isn't running. Start `python server.py` |
| ESP32 not receiving notifications | Check that `API_HOST` in config.h matches the PC's IP |
| Notifications not clearing | Each GET clears the queue — something else might be polling |
| Queue fills up (max 10) | ESP32 isn't polling, or network issue. Check `GET /api/status` |
| CORS errors from browser | Bridge sends `Access-Control-Allow-Origin: *` — should work |
| Port already in use | Another service on 8222. Use `--port 9000` or kill the other process |
| Bridge works locally but not from ESP32 | Firewall blocking port 8222. Add an inbound rule |

### Firewall (Windows)

If the ESP32 can't reach the bridge:

```powershell
# Allow inbound on port 8222
New-NetFirewallRule -DisplayName "Red Device Bridge" -Direction Inbound -LocalPort 8222 -Protocol TCP -Action Allow
```

### Testing Connectivity

```bash
# From another machine on the same network
curl http://192.168.1.100:8222/api/status

# Should return JSON with "status": "running"
```

---

## Architecture Notes

- **Thread-safe**: Queue protected by `threading.Lock()`
- **CORS enabled**: `Access-Control-Allow-Origin: *` on all responses
- **Stateless delivery**: Queue clears on each poll (fire-and-forget)
- **Auto-timestamping**: If `time` not provided, uses current `HH:MM`
- **Graceful limits**: Queue caps at 10 — excess silently dropped
- **Zero dependencies**: Only Python 3.6+ stdlib (`http.server`, `json`, `argparse`, `threading`)

---

*Bridge docs maintained by Red 🦞*
