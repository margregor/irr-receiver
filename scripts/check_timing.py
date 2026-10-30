"""Fail a build when the implementation no longer meets its clock target."""

from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    report = Path(sys.argv[1])
    required_mhz = float(sys.argv[2])
    text = report.read_text(encoding="utf-8", errors="ignore")
    match = re.search(
        r"Actual Fmax</th>.*?<tr>\s*<td>.*?</td>\s*<td>.*?</td>\s*"
        r"<td>.*?</td>\s*<td>([0-9]+(?:\.[0-9]+)?)\s*\(MHz\)</td>",
        text,
        flags=re.DOTALL,
    )
    if match is None:
        print(f"[TIMING FAILED] Could not find Actual Fmax in {report}")
        return 2

    actual_mhz = float(match.group(1))
    margin_mhz = actual_mhz - required_mhz
    print(f"[TIMING] Fmax {actual_mhz:.3f} MHz; required {required_mhz:.3f} MHz; margin {margin_mhz:.3f} MHz")
    if actual_mhz < required_mhz:
        print("[TIMING FAILED] Implementation does not meet the FT232H clock.")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
