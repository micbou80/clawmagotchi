# 🌉 Red Device — Notification Bridge

HTTP server that sits between Clawpilot and the ESP32 ClawMagotchi device.

## How It Works

```
Clawpilot ──POST──► Bridge (port 8222) ◄──GET── ESP32 Device
                    holds queue in memory
```

1. **Clawpilot** pushes notifications via `POST /api/notifications`
2. **Bridge** holds them in a queue (max 10)
3. **ESP32** polls `GET /api/notifications` every 5 seconds
4. Poll response returns all queued items and clears the queue

## Quick Start

```bash
# No dependencies needed — pure Python 3 stdlib
python server.py

# Custom port
python server.py --port 9000
```

## API

### `GET /api/notifications` — Device poll endpoint

Returns queued notifications and **clears the queue**.

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

### `POST /api/notifications` — Push a notification

Send a single notification or an array.

```bash
curl -X POST http://localhost:8222/api/notifications \
  -H "Content-Type: application/json" \
  -d '{"type":"email","title":"Chris Luce","body":"Budget review ready"}'
```

**Notification types:** `email`, `teams`, `calendar`, `urgent`, `info`

### `GET /api/status` — Health check

```json
{
  "status": "running",
  "queue_size": 2,
  "total_pushed": 15,
  "total_polled": 42,
  "device_ip": "192.168.1.50"
}
```

### `POST /api/clear` — Clear queue

Removes all queued notifications without delivering them.

## Testing

```bash
# Start the bridge
python server.py

# In another terminal, push test notifications
python test_push.py
```

## Config in ClawMagotchi

Set these in `config.h` on the ESP32:

```c
#define API_HOST      "192.168.1.100"   // IP of this PC
#define API_PORT      8222
#define API_PATH      "/api/notifications"
#define POLL_INTERVAL 5000              // ms between polls
```
