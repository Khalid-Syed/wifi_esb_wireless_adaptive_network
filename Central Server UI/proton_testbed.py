"""Thin launcher for the PROTON Testbed GUI.

Author: Md Sadman Siraj
Email: msiraj13@asu.edu
Date: 2026-02-02

This file is intentionally small to keep PyInstaller and legacy workflows simple.
The application code lives in `src/proton_testbed_gui/`.

Running without installation
----------------------------
If you run this file directly (e.g. `python proton_testbed.py`) without
installing the package, this launcher adds the local `src/` directory to
`sys.path` so imports resolve correctly.
"""

from __future__ import annotations

import sys
from pathlib import Path


# Ensure `src/` is importable when running from a fresh checkout.
_ROOT = Path(__file__).resolve().parent
_SRC = _ROOT / "src"
if _SRC.is_dir():
    sys.path.insert(0, str(_SRC))

from proton_testbed_gui.__main__ import main  # noqa: E402  (import after sys.path fix)


if __name__ == "__main__":
    raise SystemExit(main())
