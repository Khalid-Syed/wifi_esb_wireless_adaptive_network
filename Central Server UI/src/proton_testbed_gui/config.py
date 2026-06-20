"""Project configuration.

Author: Md Sadman Siraj
Email: msiraj13@asu.edu
Date: 2026-02-02

Notes:
- Values here match the original project defaults.
- You can override some settings via environment variables.
"""

from __future__ import annotations

import os

FETCH_DEVICES_TIMEOUT_MS: int = int(os.getenv("FETCH_DEVICES_TIMEOUT_MS", "2000"))

HOST: str = os.getenv("PROTON_HOST", "0.0.0.0")
PORT: int = int(os.getenv("PROTON_PORT", "12530"))

BROKER_ADDRESS: str = os.getenv("PROTON_MQTT_HOST", "127.0.0.1")
BROKER_PORT: int = int(os.getenv("PROTON_MQTT_PORT", "1883"))

SUBSCRIBE_TOPIC: str = os.getenv("PROTON_MQTT_SUBSCRIBE", "#")

MOSQUITTO_SERVICE_NAME: str = os.getenv("PROTON_MOSQUITTO_SERVICE", "mosquitto")
