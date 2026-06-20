"""Qt logging helpers.

Author: Md Sadman Siraj
Email: msiraj13@asu.edu
Date: 2026-02-02

This module provides a thread-safe logging handler that forwards log messages to a
QTextEdit widget via Qt signals.
"""

from __future__ import annotations

import logging
from PyQt5.QtCore import QObject, pyqtSignal


class QTextEditLogger(logging.Handler, QObject):
    """A logging handler that appends formatted log lines to a QTextEdit."""

    log_signal = pyqtSignal(str)

    def __init__(self, text_edit):
        logging.Handler.__init__(self)
        QObject.__init__(self)
        self._widget = text_edit
        self.log_signal.connect(self._append)

    def emit(self, record: logging.LogRecord) -> None:
        msg = self.format(record)
        self.log_signal.emit(msg)

    def _append(self, msg: str) -> None:
        # Keep the log widget performance stable by limiting the number of lines
        if self._widget.document().blockCount() > 500:
            cursor = self._widget.textCursor()
            cursor.movePosition(cursor.Start)
            cursor.select(cursor.BlockUnderCursor)
            cursor.removeSelectedText()
            cursor.deleteChar() # removes newline
            
        self._widget.append(msg)
        self._widget.ensureCursorVisible()
