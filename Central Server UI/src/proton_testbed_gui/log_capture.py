"""Log capture utilities.

Author: Md Sadman Siraj
Email: msiraj13@asu.edu
Date: 2026-02-02

This module captures selected device telemetry payloads into a timestamped log
folder under `logs/`.

Behavior is preserved from the original project:
- A new `logs/log_YYYY-MM-DD_HH-MM-SS/` folder is created when capture starts
- A `node_list.txt` is written with a numbered list of device IDs
- A `rtt_results.csv` is written with RTT metrics
"""

from __future__ import annotations

import csv
import logging
import os
import sys
from dataclasses import dataclass
from datetime import datetime
from typing import Any, Dict, Iterable, Optional


def _normalize_device_id(dev: Any) -> str:
    """Return a stable string device ID.

    The original code occasionally handled byte-string-like IDs; we keep
    compatibility by normalizing here.
    """
    if isinstance(dev, bytes):
        try:
            return dev.decode("utf-8", errors="replace")
        except Exception:
            return str(dev)

    s = str(dev)
    # Strip common representations like "b'AA:BB:...'
    if s.startswith("b'") and s.endswith("'"):
        return s[2:-1]
    return s


def get_deep_size(obj: Any, seen: Optional[set[int]] = None) -> int:
    """Approximate the deep size (in bytes) of nested containers."""
    if seen is None:
        seen = set()

    obj_id = id(obj)
    if obj_id in seen:
        return 0
    seen.add(obj_id)

    size = sys.getsizeof(obj)

    if isinstance(obj, dict):
        size += sum(get_deep_size(k, seen) + get_deep_size(v, seen) for k, v in obj.items())
    elif isinstance(obj, (list, tuple, set)):
        size += sum(get_deep_size(i, seen) for i in obj)

    return size


@dataclass
class LogPaths:
    log_dir: str
    node_list_path: str
    rtt_csv_path: str


class LogCapture:
    """Capture telemetry payloads and RTT measurements into CSV files."""

    def __init__(self, mac_ids: Iterable[Any]):
        now = datetime.now()
        timestamp = now.strftime("%Y-%m-%d_%H-%M-%S")

        log_dir = os.path.join("logs", f"log_{timestamp}")
        os.makedirs(log_dir, exist_ok=True)
        logging.info("Log directory created: %s", log_dir)

        self.paths = LogPaths(
            log_dir=log_dir,
            node_list_path=os.path.join(log_dir, "node_list.txt"),
            rtt_csv_path=os.path.join(log_dir, "rtt_results.csv"),
        )

        # Normalize and freeze device list ordering for stable numbering.
        self.device_list = sorted({_normalize_device_id(d) for d in mac_ids})
        self.device_dict: Dict[str, int] = {dev: 1 for dev in self.device_list}
        self.device_slno: Dict[str, int] = {dev: i + 1 for i, dev in enumerate(self.device_list)}

        self._txt_file = open(self.paths.node_list_path, "w", newline="", encoding="utf-8")
        for i, dev in enumerate(self.device_list, start=1):
            self._txt_file.write(f"{i} - {dev}\n")
        self._txt_file.flush()

        # Initialize RTT CSV
        self._rtt_file = open(self.paths.rtt_csv_path, "w", newline="", encoding="utf-8")
        self._rtt_writer = csv.writer(self._rtt_file)
        self._rtt_writer.writerow(
            [
                "Packet #",
                "Device ID",
                "TX Timestamp",
                "RX Timestamp",
                "RTT Latency [ms]",
                "Data Rate [Kbps]",
                "Response",
                "Latest IPC Msg",
            ]
        )
        self._rtt_file.flush()

    def close(self) -> None:
        """Close any open file handles."""
        try:
            if getattr(self, "_rtt_file", None):
                self._rtt_file.close()
        finally:
            if getattr(self, "_txt_file", None):
                self._txt_file.close()

    def __del__(self):
        # Best-effort cleanup.
        try:
            self.close()
        except Exception:
            pass

    def capture(
        self,
        payload: Dict[str, Any],
        mac_id: Any,
        latency_override: Optional[float | str] = None,
        data_rate_override: Optional[float | str] = None,
    ) -> None:
        """Legacy API retained; node_data.csv generation has been removed."""
        return

    def capture_rtt(
        self,
        pkt_num: int,
        dev_id: str,
        tx_ts: str,
        rx_ts: str,
        latency: float | str,
        data_rate_kbps: float,
        response: str,
        ipc_msg: str = "",
    ) -> None:
        """Append an RTT measurement row to the rtt_results.csv."""
        row = [pkt_num, dev_id, tx_ts, rx_ts, latency, data_rate_kbps, response, ipc_msg]
        self._rtt_writer.writerow(row)

    def capture_latency(
        self,
        dev_id: str,
        latency: float | str,
        data_rate: float | str,
        ipc_msg: str = "",
    ) -> None:
        """Legacy API retained; latency_results.csv generation has been removed."""
        return
        