"""Module entrypoint.

Author: Md Sadman Siraj
Email: msiraj13@asu.edu
Date: 2026-02-02
"""

from __future__ import annotations

import sys

from PyQt5.QtWidgets import QApplication

from .gui import MyWindow


def main() -> int:
    app = QApplication(sys.argv)
    window = MyWindow()
    window.show()
    return app.exec_()


if __name__ == "__main__":
    raise SystemExit(main())
