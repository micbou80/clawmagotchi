"""
Push a notification to the Red Device bridge.

Usage:
  python push.py --type email --title "Chris Luce" --body "Budget review ready"
  python push.py --type urgent --title "Meeting NOW" --body "Room changed"
  python push.py --type teams --title "Dream Team" --body "Julien shared agenda"

Or from Python:
  from push import push_notification
  push_notification("email", "Chris Luce", "Budget review ready")
"""

import json
import sys
import argparse
import urllib.request
import urllib.error
from datetime import datetime

BRIDGE_URL = "http://localhost:8222/api/notifications"


def push_notification(ntype="info", title="", body="", time_str=None, bridge_url=BRIDGE_URL):
    """Push a single notification to the bridge. Returns True on success."""
    if time_str is None:
        time_str = datetime.now().strftime("%H:%M")

    payload = json.dumps({
        "type": ntype,
        "title": title,
        "body": body,
        "time": time_str,
    }).encode()

    req = urllib.request.Request(
        bridge_url,
        data=payload,
        headers={"Content-Type": "application/json"},
    )

    try:
        resp = urllib.request.urlopen(req, timeout=3)
        result = json.loads(resp.read())
        return result.get("queued", 0) > 0
    except (urllib.error.URLError, TimeoutError):
        return False


def is_bridge_running(bridge_url=BRIDGE_URL):
    """Check if the bridge server is reachable."""
    try:
        status_url = bridge_url.rsplit("/", 1)[0] + "/status"
        resp = urllib.request.urlopen(status_url, timeout=2)
        return resp.status == 200
    except Exception:
        return False


def main():
    parser = argparse.ArgumentParser(description="Push notification to Red Device")
    parser.add_argument("--type", default="info", choices=["email", "teams", "calendar", "urgent", "info"])
    parser.add_argument("--title", required=True)
    parser.add_argument("--body", default="")
    parser.add_argument("--time", default=None)
    parser.add_argument("--url", default=BRIDGE_URL)
    args = parser.parse_args()

    ok = push_notification(args.type, args.title, args.body, args.time, args.url)
    if ok:
        print(f"✅ Pushed [{args.type}] {args.title}")
    else:
        print(f"❌ Bridge not reachable at {args.url}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
