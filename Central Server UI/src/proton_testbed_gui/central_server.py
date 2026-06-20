"""Central Server: TCP UTC time sync + MQTT listener/publisher.

Author: Md Sadman Siraj
Email: msiraj13@asu.edu
Date: 2026-02-02

This module preserves the original behavior:
- TCP time server (sends a binary UTC timestamp packet)
- MQTT subscriber to all topics (`#`)
- `publish_message()` helper for the GUI
- `start_CS()` / `stop_CS()` helpers for lifecycle management
- Optional log capture of device telemetry when enabled

Implementation cleanups in this refactor:
- Removed unused/duplicate imports
- Added cross-platform handling for Mosquitto service control (Windows only)
- Added helper functions to start/stop log capture safely
"""

from __future__ import annotations

import ctypes
import json
import logging
import os
import socket
import subprocess
import sys
import threading
import time
import queue
from datetime import datetime
from typing import Optional

import paho.mqtt.client as mqtt

from .config import (
    BROKER_ADDRESS,
    BROKER_PORT,
    HOST,
    MOSQUITTO_SERVICE_NAME,
    PORT,
    SUBSCRIBE_TOPIC,
)
from .log_capture import LogCapture

# === Public state (kept for backwards compatibility with the original code) ===

MAC_IDS: set[str] = set()
# Alias using original naming in the legacy code
MAC_ids = MAC_IDS

START_CAPTURE: bool = False
DIR_CREATED: bool = False
STOP_CAPTURE: bool = False

# === Internal runtime handles ===
_server_running: bool = True
_mqtt_client: Optional[mqtt.Client] = None
_tcp_socket: Optional[socket.socket] = None
_log_obj: Optional[LogCapture] = None

# RTT Test State
_rtt_test_active: bool = False
_current_packet_num: int = 0
_total_rtt_packets: int = 0
_rtt_interval_ms: int = 500
_last_tx_timestamp: Optional[datetime] = None
_responded_in_current_iteration: bool = False

# New experiment tracking
_last_payload_arrival_times: Dict[str, datetime] = {}
_experiment_monitor_thread: Optional[threading.Thread] = None

# Message processing queue for high-rate data
_message_queue: queue.Queue = queue.Queue()
_worker_thread: Optional[threading.Thread] = None

# Performance optimization flag
_ui_logging_disabled: bool = False


# === Time Packet Structure (binary) ===
class TimePacket(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_uint8),
        ("board_id", ctypes.c_uint8),
        ("year", ctypes.c_uint16),
        ("month", ctypes.c_uint8),
        ("day", ctypes.c_uint8),
        ("hour", ctypes.c_uint8),
        ("minute", ctypes.c_uint8),
        ("second", ctypes.c_uint8),
        ("usec", ctypes.c_uint32),
    ]


def reset_mac_ids() -> None:
    """Clear the discovered MAC/device ID set."""
    MAC_IDS.clear()


# === Mosquitto service control ===
def _is_windows() -> bool:
    return sys.platform.startswith("win")


def _start_mosquitto_service() -> None:
    """Best-effort attempt to start Mosquitto.

    Original project used Windows service control via `net start mosquitto`.
    We keep that behavior on Windows; on other OSes we only log a warning.
    """
    if not _is_windows():
        logging.info(
            "Mosquitto auto-start is only supported on Windows. Ensure the broker is running on %s:%s.",
            BROKER_ADDRESS,
            BROKER_PORT,
        )
        return

    try:
        subprocess.run(
            ["net", "start", MOSQUITTO_SERVICE_NAME],
            capture_output=True,
            text=True,
            check=False,
            shell=True,
        )
    except Exception:
        logging.exception("Failed to start Mosquitto service (non-fatal).")


def _stop_mosquitto_service() -> None:
    if not _is_windows():
        return

    try:
        subprocess.run(
            ["net", "stop", MOSQUITTO_SERVICE_NAME],
            capture_output=True,
            text=True,
            check=False,
            shell=True,
        )
    except Exception:
        logging.exception("Failed to stop Mosquitto service (non-fatal).")


# === TCP Handler ===
def _handle_client(conn: socket.socket, addr) -> None:
    logging.info("Connected by: %s", addr)
    try:
        now = datetime.utcnow()
        time_packet = TimePacket(
            type=1,
            board_id=0,
            year=now.year,
            month=now.month,
            day=now.day,
            hour=now.hour,
            minute=now.minute,
            second=now.second,
            usec=now.microsecond,
        )
        conn.sendall(bytes(time_packet))
    except Exception:
        logging.exception("Error handling client %s", addr)
    finally:
        try:
            conn.close()
        except Exception:
            pass
        logging.info("Connection closed for: %s", addr)


def _start_tcp_server() -> None:
    global _tcp_socket, _server_running

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((HOST, PORT))
    sock.listen()
    _tcp_socket = sock

    logging.info("UTC Timer server started on PORT: %s", PORT)

    while _server_running:
        try:
            sock.settimeout(1.0)
            conn, addr = sock.accept()
            threading.Thread(target=_handle_client, args=(conn, addr), daemon=True).start()
        except socket.timeout:
            continue
        except Exception:
            logging.exception("TCP Server error")
            break

    try:
        sock.close()
    except Exception:
        pass
    logging.info("TCP socket closed.")


def _run_tcp_server_in_background() -> None:
    threading.Thread(target=_start_tcp_server, daemon=True).start()
    logging.info("TCP server thread started in background.")


def _experiment_timeout_monitor():
    """Monitor payload arrivals (no longer logging timeouts to CSV)."""
    global _last_payload_arrival_times, _log_obj
    while _server_running:
        # Periodic CSV logging on timeout removed as per user request.
        time.sleep(1.0)


# === MQTT callbacks ===
def _on_connect(client: mqtt.Client, userdata, flags, rc):
    if rc == 0:
        logging.info("Connected to MQTT broker")
        client.subscribe(SUBSCRIBE_TOPIC)
        logging.info("Subscribed to topic: %s", SUBSCRIBE_TOPIC)
    else:
        logging.error("Failed to connect to MQTT broker. Return code %s", rc)


def _maybe_record_device_id(topic: str, payload_bytes: bytes, payload: Optional[dict] = None) -> None:
    """Update MAC_IDS if the incoming message contains node_id."""
    if not any(t in topic for t in ["dt/", "res", "ncla"]):
        return

    if payload is None:
        try:
            payload_str = payload_bytes.decode("latin-1")
            payload = json.loads(payload_str, strict=False)
        except Exception:
            return

    node_id = payload.get("node_id")
    if node_id is None:
        # Fallback to topic parsing if node_id is missing from JSON
        parts = topic.split("/")
        if len(parts) >= 3:
            node_id = parts[2]
        else:
            return

    # Normalize as string
    if isinstance(node_id, bytes):
        try:
            node_id = node_id.decode("utf-8", errors="replace")
        except Exception:
            node_id = str(node_id)

    normalized_id = str(node_id).strip()
    if normalized_id.lower() in {"", "node_id", "unknown", "none", "null"}:
        return

    MAC_IDS.add(normalized_id)


def _ensure_log_obj() -> bool:
    """Lazy initialization of the LogCapture object and directory."""
    global DIR_CREATED, _log_obj, START_CAPTURE, STOP_CAPTURE
    
    if not START_CAPTURE or STOP_CAPTURE:
        return False
        
    if not DIR_CREATED or _log_obj is None:
        _log_obj = LogCapture(MAC_IDS)
        DIR_CREATED = True
        logging.info("Lazy-loading log capture directory.")
    return True
    

def _maybe_calculate_rtt(topic: str, payload_bytes: bytes, arrival_time: datetime, payload: Optional[dict] = None) -> None:
    """Calculate RTT and log to UI and CSV if capture is active."""
    global _rtt_test_active, _last_tx_timestamp, _responded_in_current_iteration, _current_packet_num, _log_obj, _rtt_interval_ms
    
    if _rtt_test_active and _last_tx_timestamp and "rtt_res" in topic:
        # Ignore messages that arrived BEFORE the current TX timestamp (stale/delayed from previous iteration)
        if arrival_time < _last_tx_timestamp:
            return

        latency_ms = (arrival_time - _last_tx_timestamp).total_seconds() * 1000
        
        # If the packet arrived after the defined interval, treat it as a dropped/timeout packet
        if latency_ms > _rtt_interval_ms:
            logging.debug("[RTT] Ignoring late arrival (%.3f ms > %d ms)", latency_ms, _rtt_interval_ms)
            return

        _responded_in_current_iteration = True
        rx_time = arrival_time
        # Re-use the already calculated latency_ms below
        
        # Use pre-parsed payload if available
        if payload:
            dev_id = str(payload.get("node_id", "unknown"))
        else:
            # Parse device ID from JSON payload if not provided
            try:
                payload_str = payload_bytes.decode("latin-1")
                data = json.loads(payload_str, strict=False)
                dev_id = str(data.get("node_id", "unknown"))
            except Exception:
                dev_id = "unknown"
        
        payload_bits = len(payload_bytes) * 8
        # Convert bps to Kbps
        data_rate_kbps = (payload_bits / (latency_ms / 1000.0)) / 1e3 if latency_ms > 0 else 0
        
        tx_str = _last_tx_timestamp.strftime("%H:%M:%S.%f")[:-3]
        rx_str = rx_time.strftime("%H:%M:%S.%f")[:-3]

        ipc_msg = payload.get("latest_ipc_msg", "N/A") if payload else "N/A"

        # logging.info(
        #     "[RTT] Pkt: %d, ID: %s, TX: %s, RX: %s, Latency: %.3f ms, Rate: %.4f Kbps, Resp: YES, IPC: %s",
        #     _current_packet_num, dev_id, tx_str, rx_str, latency_ms, data_rate_kbps, ipc_msg
        # )

        if _ensure_log_obj():
            _log_obj.capture_rtt(
                _current_packet_num, 
                dev_id, 
                tx_str, 
                rx_str, 
                latency_ms, 
                data_rate_kbps, 
                "YES",
                ipc_msg
            )


def _maybe_capture_logs(topic: str, payload_bytes: bytes, arrival_time: datetime, payload: Optional[dict] = None) -> None:
    """Capture telemetry messages if capture has been enabled."""
    global DIR_CREATED, _log_obj, _last_payload_arrival_times, _rtt_test_active

    # Do not log inter-arrival latency or telemetry if an RTT test is active
    # to avoid redundant/mixed data in CSVs.
    if _rtt_test_active:
        return

    if not any(t in topic for t in ["rtt_res", "dt/", "res", "ncla"]):
        return

    if not START_CAPTURE:
        return

    if payload is None:
        try:
            # Some payloads contain binary data in strings (e.g. latest_ipc_msg)
            # We decode with latin-1 to preserve all byte values and use strict=False
            # to allow control characters in JSON strings.
            payload_str = payload_bytes.decode("latin-1")
            payload = json.loads(payload_str, strict=False)
        except Exception:
            logging.debug("Telemetry payload was not valid JSON; skipping capture.")
            return

    # Use node_id from payload if available, else fallback to topic parsing
    dev = str(payload.get("node_id", "unknown"))
    if dev == "unknown":
        dev = topic.split("/")[2] if len(topic.split("/")) > 2 else "unknown"

    if _ensure_log_obj():
        # Calculate inter-arrival latency
        now = arrival_time
        latency_val = "N/A"
        data_rate_val = "N/A"

        if dev in _last_payload_arrival_times:
            diff_ms = (now - _last_payload_arrival_times[dev]).total_seconds() * 1000
            latency_val = round(diff_ms, 3)
            
            # Kbps = bits / ms
            payload_bits = len(payload_bytes) * 8
            if diff_ms > 0:
                data_rate_val = round(payload_bits / diff_ms, 4)
            else:
                data_rate_val = 0.0

            # Keep runtime latency alerting (without writing extra CSV files)
            if "ncla" in topic:
                ipc_raw = payload.get("latest_ipc_msg", "")
                
                # Experiment-specific 250ms latency check
                if isinstance(latency_val, (int, float)) and latency_val > 250:
                    logging.warning("LATENCY ALERT: Device %s inter-arrival time: %s ms (> 250ms)", dev, latency_val)

        _last_payload_arrival_times[dev] = now
        
        # Disabled logging to other CSVs as per user request
        # _log_obj.capture(payload, dev)


def _message_worker() -> None:
    """Worker thread that processes messages from the queue to decouple MQTT reception."""
    while _server_running:
        try:
            # Short timeout so we can check _server_running flag
            item = _message_queue.get(timeout=0.5)
            if item is None:
                break
            
            topic, payload_bytes, arrival_time = item
            
            # Heavy lifting: JSON parsing and logging
            try:
                payload_str = payload_bytes.decode("latin-1")
                payload = json.loads(payload_str, strict=False)
            except Exception:
                payload = None
                try:
                    payload_str = payload_bytes.decode("utf-8")
                except UnicodeDecodeError:
                    payload_str = str(payload_bytes)

            # Show in UI if not high-volume dtr, OR if it's explicitly a response.
            if not _ui_logging_disabled:
                if "dtr" not in topic or "res" in topic:
                    logging.info("---------- UI LOG ----------")
                    logging.info("TOPIC : %s", topic)
                    if payload and "latest_ipc_msg" in payload:
                        ipc_status = "Received" if payload.get("latest_ipc_msg") else "N/A"
                        logging.info("IPC STATUS : %s", ipc_status)
                    logging.info("PAYLOAD : %s", payload_str)

            _maybe_record_device_id(topic, payload_bytes, payload)
            _maybe_calculate_rtt(topic, payload_bytes, arrival_time, payload)
            _maybe_capture_logs(topic, payload_bytes, arrival_time, payload)
            
            _message_queue.task_done()
        except queue.Empty:
            continue
        except Exception:
            logging.exception("Error in MQTT worker thread")


def _on_message(client: mqtt.Client, userdata, msg) -> None:
    # Capture arrival time IMMEDIATELY on the MQTT network thread
    arrival_time = datetime.utcnow()
    # Queue the message for worker thread to process (parsing, logging, CSV)
    _message_queue.put((msg.topic, msg.payload, arrival_time))


# === Public API used by the GUI ===
def publish_message(topic: str, message: str) -> None:
    """Publish an MQTT message using the running client."""
    global _mqtt_client

    if _mqtt_client is None:
        logging.warning("MQTT client not initialized. Did you call start_CS()?")
        return

    result = _mqtt_client.publish(topic, message)
    status = result[0]
    if status == 0:
        logging.info("Published `%s` to topic `%s`", message, topic)
    else:
        logging.warning("Failed to publish message to `%s`. Result: %s", topic, status)


def set_ui_logging(enabled: bool) -> None:
    """Enable or disable terminal UI logging for performance."""
    global _ui_logging_disabled
    _ui_logging_disabled = not enabled
    state = "ENABLED" if enabled else "DISABLED"
    logging.info("Terminal UI logging is now %s", state)


def enable_log_capture() -> None:
    """Enable telemetry capture and create log directory immediately."""
    global START_CAPTURE, STOP_CAPTURE, DIR_CREATED, _log_obj, _last_payload_arrival_times

    START_CAPTURE = True
    STOP_CAPTURE = False
    _last_payload_arrival_times.clear()

    # Close any previous capture object if it existed.
    if _log_obj is not None:
        try:
            _log_obj.close()
        except Exception:
            pass
        _log_obj = None

    # Instantiate immediately so the directory is created on disk as soon as user clicks
    _log_obj = LogCapture(MAC_IDS)
    DIR_CREATED = True
    logging.info("Log capture enabled.")


def disable_log_capture() -> None:
    global START_CAPTURE, STOP_CAPTURE, DIR_CREATED, _log_obj

    STOP_CAPTURE = True
    START_CAPTURE = False
    DIR_CREATED = False

    if _log_obj is not None:
        try:
            _log_obj.close()
        except Exception:
            pass
        _log_obj = None


def start_CS() -> None:
    """Start the central server (TCP + MQTT)."""
    global _mqtt_client, _server_running, _experiment_monitor_thread, _worker_thread

    _start_mosquitto_service()

    _server_running = True
    _run_tcp_server_in_background()
    
    # Start the experiment timeout monitor
    _experiment_monitor_thread = threading.Thread(target=_experiment_timeout_monitor, daemon=True)
    _experiment_monitor_thread.start()

    # Start the high-rate message processing worker
    _worker_thread = threading.Thread(target=_message_worker, daemon=True)
    _worker_thread.start()

    client = mqtt.Client()
    client.on_connect = _on_connect
    client.on_message = _on_message
    client.connect(BROKER_ADDRESS, BROKER_PORT, 60)
    client.loop_start()

    _mqtt_client = client
    logging.info("Central Server is running.")


def stop_CS() -> None:
    """Stop the central server (TCP + MQTT)."""
    global _server_running, _mqtt_client, _tcp_socket, _worker_thread

    logging.info("Shutting down Central Server...")
    _server_running = False

    # Signify the worker thread to exit
    _message_queue.put(None)
    if _worker_thread is not None:
        _worker_thread.join(timeout=1.0)
        _worker_thread = None

    if _mqtt_client is not None:
        try:
            _mqtt_client.loop_stop()
            _mqtt_client.disconnect()
            logging.info("MQTT client disconnected.")
        except Exception:
            logging.exception("Error stopping MQTT client")
        finally:
            _mqtt_client = None

    if _tcp_socket is not None:
        try:
            _tcp_socket.close()
            logging.info("TCP socket closed.")
        except Exception:
            logging.exception("Error closing TCP socket")
        finally:
            _tcp_socket = None

    _stop_mosquitto_service()


def run_automated_rtt_test(iterations: int, interval_ms: int = 500) -> None:
    """Orchestrates the automated RTT broadcast loop."""
    global _rtt_test_active, _last_tx_timestamp, _current_packet_num, _responded_in_current_iteration, _total_rtt_packets, _rtt_interval_ms

    # Clear stale messages from the queue before beginning the RTT experiment
    _clear_message_queue()

    def _test_loop():
        global _rtt_test_active, _last_tx_timestamp, _current_packet_num, _responded_in_current_iteration, _total_rtt_packets, _rtt_interval_ms
        _rtt_test_active = True
        _total_rtt_packets = iterations
        _rtt_interval_ms = interval_ms
        
        # Consistent payload with the node firmware expectations
        # Config 1
        base_payload = [0x91, 0x01, 0x00, 0x00, 0x11, 0x12, 0x22, 0x13, 0x23, 0x33, 0x43]
        # Config 2
        base_payload = [0x91, 0x01, 0x00, 0x00, 0x11, 0x13, 0x23, 0x12, 0x22, 0x32, 0x42]
        # Config 3
        base_payload = [0x91, 0x01, 0x00, 0x00, 0x13, 0x12, 0x22, 0x11, 0x21, 0x31, 0x41]

        for i in range(1, iterations + 1):
            if not _rtt_test_active or not _server_running:
                break
            
            _current_packet_num = i
            _responded_in_current_iteration = False
            _last_tx_timestamp = datetime.utcnow()
            
            # Update the second byte (index 1) based on the iteration number
            # Using (i - 1) % 256 to ensure it wraps around if it exceeds 0xFF
            dynamic_byte = (base_payload[1] + (i - 1)) & 0xFF
            current_payload = list(base_payload)
            current_payload[1] = dynamic_byte
            
            # Format as "0x## 0x## ..." string
            payload_str_val = " ".join(f"0x{b:02x}" for b in current_payload)

            payload_dict = {
                "session_id": 99,
                "res_topic": "rtt_res",
                "operation": "RTT",
                "payload": payload_str_val
            }
            msg_str = json.dumps(payload_dict)
            
            # 1. Publish broadcast message
            publish_message("cmd/proton-node-test/rtt", msg_str)
            
            # 6. Wait for specified interval (e.g. 500ms)
            time.sleep(interval_ms / 1000.0)
            
            # If no node responded in the 500ms window
            if not _responded_in_current_iteration:
                rx_now = datetime.utcnow()
                tx_str = _last_tx_timestamp.strftime("%H:%M:%S.%f")[:-3]
                rx_str = rx_now.strftime("%H:%M:%S.%f")[:-3]
                
                # logging.info(
                #     "[RTT] Pkt: %d, ID: N/A, TX: %s, RX: %s, Latency: INF, Rate: 0, Resp: NO",
                #     _current_packet_num, tx_str, rx_str
                # )

                if DIR_CREATED and not STOP_CAPTURE and _log_obj is not None:
                    _log_obj.capture_rtt(
                        _current_packet_num, "N/A", tx_str, rx_str, "INF", 0.0, "NO", "N/A"
                    )

        _rtt_test_active = False
        _current_packet_num = 0
        logging.info("Automated RTT test finished.")

    threading.Thread(target=_test_loop, daemon=True).start()

def get_rtt_status() -> tuple[int, int, bool]:
    """Returns (current_iteration, total_iterations, is_active)."""
    return _current_packet_num, _total_rtt_packets, _rtt_test_active

def stop_automated_rtt_test() -> None:
    global _rtt_test_active
    _rtt_test_active = False


def is_mqtt_connected() -> bool:
    return bool(_mqtt_client and _mqtt_client.is_connected())


def _clear_message_queue() -> None:
    """Flush the message queue to remove stale telemetry before starting a test."""
    while not _message_queue.empty():
        try:
            _message_queue.get_nowait()
            _message_queue.task_done()
        except queue.Empty:
            break
    logging.info("Message queue cleared.")
