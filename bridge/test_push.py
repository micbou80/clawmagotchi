"""Quick test — push sample notifications to the bridge."""

import json
import urllib.request

BRIDGE = "http://localhost:8222"

samples = [
    {"type": "email", "title": "Chris Luce", "body": "FY27 budget review — need your input by EOD"},
    {"type": "teams", "title": "Dream Team", "body": "Julien shared the Milan agenda"},
    {"type": "calendar", "title": "1:1 with Albert", "body": "Starting in 15 minutes — Room 4B-201"},
    {"type": "urgent", "title": "Production alert", "body": "Teams Rooms portal returning 503 errors"},
]

for notif in samples:
    data = json.dumps(notif).encode()
    req = urllib.request.Request(
        f"{BRIDGE}/api/notifications",
        data=data,
        headers={"Content-Type": "application/json"},
    )
    resp = urllib.request.urlopen(req)
    result = json.loads(resp.read())
    print(f"✅ [{notif['type']}] {notif['title']} — queued ({result['total']} in queue)")

print("\n🦞 All test notifications pushed! Device will pick them up on next poll.")
