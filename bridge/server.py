"""
╔═════════════════════════════════════════╗
║  Red Device — Notification Bridge       ║
║  HTTP server for ClawMagotchi           ║
╚═════════════════════════════════════════╝

Sits between Clawpilot and the ESP32 device.
- POST /api/notifications  → queue a notification
- GET  /api/notifications  → device polls this (returns + clears queue)
- GET  /api/status         → bridge health check
- POST /api/clear          → manually clear all notifications

Run:  python server.py
Port: 8222 (configurable via --port)
"""

import json
import time
import argparse
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime

# Thread-safe notification queue
lock = threading.Lock()
notifications = []
stats = {
    "started": None,
    "total_pushed": 0,
    "total_polled": 0,
    "last_push": None,
    "last_poll": None,
    "device_ip": None,
}

MAX_QUEUE = 10


class BridgeHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"[{timestamp}] {args[0]}")

    def _send_json(self, code, data):
        body = json.dumps(data).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", len(body))
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        if self.path == "/api/notifications":
            self._handle_poll()
        elif self.path == "/api/status":
            self._handle_status()
        else:
            self._send_json(404, {"error": "not found"})

    def do_POST(self):
        if self.path == "/api/notifications":
            self._handle_push()
        elif self.path == "/api/clear":
            self._handle_clear()
        else:
            self._send_json(404, {"error": "not found"})

    def _handle_poll(self):
        """Device polls this — returns current queue and clears it."""
        with lock:
            result = list(notifications)
            notifications.clear()
            stats["total_polled"] += 1
            stats["last_poll"] = datetime.now().isoformat()
            stats["device_ip"] = self.client_address[0]

        self._send_json(200, {"notifications": result})

    def _handle_push(self):
        """Clawpilot pushes notifications here."""
        try:
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length)
            data = json.loads(body)
        except (json.JSONDecodeError, ValueError) as e:
            self._send_json(400, {"error": f"invalid JSON: {e}"})
            return

        # Accept single notification or array
        items = data if isinstance(data, list) else [data]

        added = 0
        with lock:
            for item in items:
                if len(notifications) >= MAX_QUEUE:
                    break
                notif = {
                    "type": item.get("type", "info"),
                    "title": item.get("title", "Notification"),
                    "body": item.get("body", ""),
                    "time": item.get("time", datetime.now().strftime("%H:%M")),
                }
                notifications.append(notif)
                added += 1
                stats["total_pushed"] += 1

            stats["last_push"] = datetime.now().isoformat()

        print(f"  📨 Queued {added} notification(s) — {len(notifications)} in queue")
        self._send_json(200, {"queued": added, "total": len(notifications)})

    def _handle_clear(self):
        """Manually clear all queued notifications."""
        with lock:
            count = len(notifications)
            notifications.clear()
        self._send_json(200, {"cleared": count})

    def _handle_status(self):
        """Health check / dashboard."""
        with lock:
            queue_size = len(notifications)
        self._send_json(200, {
            "status": "running",
            "queue_size": queue_size,
            "max_queue": MAX_QUEUE,
            **stats,
        })


def main():
    parser = argparse.ArgumentParser(description="Red Device Notification Bridge")
    parser.add_argument("--port", type=int, default=8222, help="Port to listen on")
    parser.add_argument("--host", default="0.0.0.0", help="Host to bind to")
    args = parser.parse_args()

    stats["started"] = datetime.now().isoformat()

    server = HTTPServer((args.host, args.port), BridgeHandler)
    print(f"🦞 Red Device Bridge running on http://{args.host}:{args.port}")
    print(f"   Device polls:  GET  /api/notifications")
    print(f"   Push notifs:   POST /api/notifications")
    print(f"   Health check:  GET  /api/status")
    print(f"   Clear queue:   POST /api/clear")
    print()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n🛑 Bridge stopped")
        server.server_close()


if __name__ == "__main__":
    main()
