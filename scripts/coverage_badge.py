#!/usr/bin/env python3
"""Convert a gcovr JSON summary into a shields.io endpoint JSON file."""
import json
import sys


def colour(percent: float) -> str:
    for limit, name in ((90, "brightgreen"), (80, "green"), (70, "yellowgreen"), (60, "yellow")):
        if percent >= limit:
            return name
    return "orange"


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: coverage_badge.py SUMMARY.json OUTPUT.json", file=sys.stderr)
        return 2
    with open(sys.argv[1], encoding="utf-8") as source:
        percent = float(json.load(source)["line_percent"])
    badge = {
        "schemaVersion": 1,
        "label": "coverage",
        "message": f"{percent:.0f}%",
        "color": colour(percent),
    }
    with open(sys.argv[2], "w", encoding="utf-8") as target:
        json.dump(badge, target)
    return 0


if __name__ == "__main__":
    sys.exit(main())
