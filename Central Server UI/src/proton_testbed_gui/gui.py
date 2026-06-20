"""PROTON Testbed GUI.

Author: Md Sadman Siraj
Email: msiraj13@asu.edu
Date: 2026-02-02

This module implements the PyQt5 GUI for interacting with the PROTON Testbed.

The message topics and payload structures are preserved from the original
implementation so that node firmware and existing workflows continue to work.
"""

from __future__ import annotations

import json
import logging
from typing import List, Tuple

from PyQt5.QtCore import QTimer, Qt, pyqtSignal
from PyQt5.QtWidgets import (
    QApplication,
    QCheckBox,
    QDialogButtonBox,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QPushButton,
    QStyle,
    QTabWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from .config import FETCH_DEVICES_TIMEOUT_MS
from .central_server import (
    MAC_IDS,
    disable_log_capture,
    enable_log_capture,
    get_rtt_status,
    publish_message,
    reset_mac_ids,
    run_automated_rtt_test,
    stop_automated_rtt_test,
    set_ui_logging,
    start_CS,
    stop_CS,
)
from .utils.qt_logging import QTextEditLogger


class DeviceListDialog(QWidget):
    """Small dialog listing currently selected devices before starting capture."""

    capture_confirmed = pyqtSignal()

    def __init__(self, device_checkboxes: List[Tuple[QCheckBox, str]]):
        super().__init__()
        self.setWindowTitle("Devices Capturing Logs")
        self.setGeometry(300, 300, 400, 300)

        layout = QVBoxLayout()

        self.valid_selection = False

        if not device_checkboxes:
            layout.addWidget(QLabel("No devices are currently selected or available."))
        else:
            layout.addWidget(QLabel("Devices currently selected:"))

            for checkbox, mac in device_checkboxes:
                if checkbox.isChecked():
                    layout.addWidget(QLabel(mac))
                    self.valid_selection = True

        button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        button_box.accepted.connect(self.accept_action)
        button_box.rejected.connect(self.close)
        layout.addWidget(button_box)

        self.setLayout(layout)

    def accept_action(self):
        if self.valid_selection:
            self.capture_confirmed.emit()
        self.close()


class MyWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("PROTON Testbed")
        self.setGeometry(100, 100, 1200, 800)

        # Tabs
        tabs = QTabWidget()
        tab1 = QWidget()
        tab2 = QWidget()
        tab3 = QWidget()

        tabs.addTab(tab1, "Central Server")
        tabs.addTab(tab2, "Devices")
        tabs.addTab(tab3, "Custom Payloads")

        # --- Tab 1: Central Server ---
        vbox1 = QVBoxLayout()
        vbox1.addWidget(QLabel("Central Server Logs:"))

        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)
        vbox1.addWidget(self.log_output)

        button_layout = QHBoxLayout()

        self.server_running = False
        self.device_checkboxes: List[Tuple[QCheckBox, str]] = []

        self.server_button = QPushButton("Start Central Server")
        self.start_icon = self.style().standardIcon(QStyle.SP_MediaPlay)
        self.stop_icon = self.style().standardIcon(QStyle.SP_MediaStop)
        self.server_button.setIcon(self.start_icon)
        self.server_button.clicked.connect(self.toggle_server)
        button_layout.addWidget(self.server_button)

        # Automated Loop Controls
        rtt_box = QHBoxLayout()
        
        rtt_box.addWidget(QLabel("Iterations:"))
        self.rtt_iterations_input = QTextEdit("1200")
        self.rtt_iterations_input.setFixedWidth(70)
        self.rtt_iterations_input.setFixedHeight(35)
        rtt_box.addWidget(self.rtt_iterations_input)

        rtt_box.addWidget(QLabel("Interval (ms):"))
        self.rtt_interval_input = QTextEdit("500")
        self.rtt_interval_input.setFixedWidth(70)
        self.rtt_interval_input.setFixedHeight(35)
        rtt_box.addWidget(self.rtt_interval_input)

        self.rtt_progress_label = QLabel("Progress: 0 / 0")
        rtt_box.addWidget(self.rtt_progress_label)

        self.rtt_running = False
        self.rtt_btn = QPushButton("Start RTT Loop")
        self.rtt_btn.setEnabled(False)
        self.rtt_btn.clicked.connect(self.toggle_rtt_loop)
        rtt_box.addWidget(self.rtt_btn)

        # Clear Logs button
        self.clear_logs_btn = QPushButton("Clear Logs")
        clear_icon = self.style().standardIcon(QStyle.SP_DialogDiscardButton)
        self.clear_logs_btn.setIcon(clear_icon)
        self.clear_logs_btn.clicked.connect(self.clear_terminal_logs)
        rtt_box.addWidget(self.clear_logs_btn)

        vbox1.addLayout(rtt_box)

        # Timer for live counter
        self.rtt_timer = QTimer()
        self.rtt_timer.timeout.connect(self.update_rtt_progress)

        vbox1.addLayout(button_layout)

        vbox1.addLayout(button_layout)
        tab1.setLayout(vbox1)

        # --- Tab 2: Devices ---
        vbox2 = QVBoxLayout()
        vbox2.addWidget(QLabel("Device Controls:"))

        # Log Capture button (moved to Devices tab)
        self.capturing = False
        self.start_capture_icon = self.style().standardIcon(QStyle.SP_DialogYesButton)
        self.stop_capture_icon = self.style().standardIcon(QStyle.SP_DialogCancelButton)

        self.capture_logs_btn = QPushButton("Start Log Capture")
        self.capture_logs_btn.setIcon(self.start_capture_icon)
        self.capture_logs_btn.clicked.connect(self.toggle_capture)
        self.capture_logs_btn.setEnabled(False)
        vbox2.addWidget(self.capture_logs_btn)

        # Horizontal row of buttons
        button_row = QHBoxLayout()

        # Refresh devices
        refresh_icon = self.style().standardIcon(QStyle.SP_BrowserReload)
        self.get_devices_button = QPushButton("Get Devices")
        self.get_devices_button.setIcon(refresh_icon)
        self.get_devices_button.setEnabled(False)
        self.get_devices_button.clicked.connect(self.update_device_list)
        button_row.addWidget(self.get_devices_button)

        # Firmware update
        self.firmware_button = QPushButton("Firmware Update")
        firmware_icon = self.style().standardIcon(QStyle.SP_ComputerIcon)
        self.firmware_button.setIcon(firmware_icon)
        self.firmware_button.setEnabled(False)
        self.firmware_button.clicked.connect(self.firmware_update)
        button_row.addWidget(self.firmware_button)

        # Reboot
        self.reboot_button = QPushButton("Reboot Devices")
        reboot_icon = self.style().standardIcon(QStyle.SP_BrowserStop)
        self.reboot_button.setIcon(reboot_icon)
        self.reboot_button.setEnabled(False)
        self.reboot_button.clicked.connect(self.reboot_devices)
        button_row.addWidget(self.reboot_button)

        vbox2.addLayout(button_row)

        # Device list
        vbox2.addWidget(QLabel("Connected Devices:"))
        self.device_list = QListWidget()
        self.select_all_checkbox = QCheckBox("Select All")
        self.select_all_checkbox.stateChanged.connect(self.toggle_all_checkboxes)
        self.select_all_checkbox.setEnabled(False)

        vbox2.addWidget(self.select_all_checkbox)
        vbox2.addWidget(self.device_list)

        tab2.setLayout(vbox2)

        # --- Tab 3: Custom Payloads ---
        vbox3 = QVBoxLayout()
        vbox3.addWidget(QLabel("MQTT payload : "))
        vbox3.setContentsMargins(10, 10, 10, 10)
        vbox3.setSpacing(5)

        self.json_input = QTextEdit()
        self.json_input.setPlaceholderText("Paste JSON log here...")
        self.json_input.setFixedHeight(150)
        vbox3.addWidget(self.json_input)

        topic_label = QLabel("MQTT topic : ")
        vbox3.addWidget(topic_label)

        self.topic_input = QTextEdit()
        self.topic_input.setPlaceholderText("Enter MQTT topic (e.g., cmd/proton-node-test/custom_topic)")
        self.topic_input.setFixedHeight(40)
        vbox3.addWidget(self.topic_input)

        self.submit_json_btn = QPushButton("Publish")
        self.submit_json_btn.clicked.connect(self.handle_json_input)
        self.submit_json_btn.setEnabled(False)
        vbox3.addWidget(self.submit_json_btn)

        vbox3.addStretch()
        tab3.setLayout(vbox3)

        self.setCentralWidget(tabs)

        # Setup logging
        self.setup_logger()

        # Apply dark theme
        self.apply_dark_theme()

        # variable for FOTA session ID:
        self.Session_ID = 1

        # Loading devices animation
        self.loading_timer = QTimer()
        self.loading_dots = 0
        self.loading_timer.timeout.connect(self.animate_loading_text)

    # --- Theming ---
    def apply_dark_theme(self):
        self.setStyleSheet(
            """
            QWidget {
                background-color: #2b2b2b;
                color: #ffffff;
                font-family: Segoe UI, sans-serif;
                font-size: 11pt;
            }

            QPushButton {
                background-color: #3c3f41;
                border: 1px solid #5c5c5c;
                padding: 5px;
                border-radius: 4px;
            }

            QPushButton:hover {
                background-color: #505357;
            }

            QPushButton:disabled {
                background-color: #2e2e2e;
                color: #6d6d6d;
            }

            QTabWidget::pane {
                border: 1px solid #444;
            }

            QTabBar::tab {
                background: #3c3f41;
                border: 1px solid #5c5c5c;
                padding: 5px;
            }

            QTabBar::tab:selected {
                background: #505357;
            }

            QTextEdit {
                background-color: #1e1e1e;
                color: #cfcfcf;
                border: 1px solid #5c5c5c;
            }

            QLabel {
                color: #ffffff;
            }
        """
        )

    # --- Logging helpers ---
    def setup_logger(self):
        log_handler = QTextEditLogger(self.log_output)
        log_handler.setFormatter(
            logging.Formatter('<span style="color:#33cc33;">[PROTON Testbed]</span> %(message)s')
        )

        root = logging.getLogger()
        root.addHandler(log_handler)
        root.setLevel(logging.DEBUG)

    def log_info(self, message: str) -> None:
        logging.info(message)

    def log_warn(self, message: str) -> None:
        logging.warning(message)

    def log_error(self, message: str) -> None:
        logging.error(message)

    def clear_terminal_logs(self):
        """Clear the visual logs in the QTextEdit terminal."""
        self.log_output.clear()
        self.log_info("Terminal cleared.")

    # --- Central Server controls ---
    def toggle_server(self):
        if not self.server_running:
            start_CS()
            self.log_info("CS up and running ..........")
            self.server_button.setText("Stop Central Server")
            self.server_button.setIcon(self.stop_icon)
            self.server_running = True

            # Enable related controls
            self.get_devices_button.setEnabled(True)
            self.firmware_button.setEnabled(True)
            self.reboot_button.setEnabled(True)
            self.select_all_checkbox.setEnabled(True)
            self.submit_json_btn.setEnabled(True)
            self.capture_logs_btn.setEnabled(True)
            self.rtt_btn.setEnabled(True)
        else:
            stop_CS()
            self.log_info("CS is stopped ..........")
            self.server_button.setText("Start Central Server")
            self.server_button.setIcon(self.start_icon)
            self.server_running = False

            # Disable related controls
            self.get_devices_button.setEnabled(False)
            self.firmware_button.setEnabled(False)
            self.reboot_button.setEnabled(False)
            self.select_all_checkbox.setEnabled(False)
            self.submit_json_btn.setEnabled(False)
            self.capture_logs_btn.setEnabled(False)
            self.rtt_btn.setEnabled(False)

    # Automated RTT Loop
    def toggle_rtt_loop(self):
        if not self.rtt_running:
            try:
                iters = int(self.rtt_iterations_input.toPlainText().strip())
                interval = int(self.rtt_interval_input.toPlainText().strip())
                run_automated_rtt_test(iterations=iters, interval_ms=interval)
                self.rtt_btn.setText("Stop RTT Loop")
                self.rtt_running = True
                self.rtt_timer.start(100)  # Check status every 100ms
            except ValueError:
                self.log_error("Iterations and Interval must be valid integers.")
        else:
            stop_automated_rtt_test()
            self.rtt_btn.setText("Start RTT Loop")
            self.rtt_running = False
            self.rtt_timer.stop()

    def update_rtt_progress(self):
        current, total, active = get_rtt_status()
        self.rtt_progress_label.setText(f"Progress: {current} / {total}")
        
        # If the background thread has finished but the GUI still thinks it's running
        if not active and self.rtt_running:
            self.rtt_btn.setText("Start RTT Loop")
            self.rtt_running = False
            self.rtt_timer.stop()
            self.log_info("RTT Loop completed successfully.")

    # --- Log capture controls ---
    def begin_log_capture(self):
        enable_log_capture()
        set_ui_logging(False) # Disable UI logging during CSV capture
        self.capturing = True
        self.capture_logs_btn.setText("Stop Capture")
        self.capture_logs_btn.setIcon(self.stop_capture_icon)
        self.log_info("Log capture started.")

    def toggle_capture(self):
        if not self.capturing:
            self.device_dialog = DeviceListDialog(self.device_checkboxes)
            self.device_dialog.capture_confirmed.connect(self.begin_log_capture)
            self.device_dialog.show()
        else:
            disable_log_capture()
            set_ui_logging(True) # Re-enable UI logging
            self.capturing = False
            self.capture_logs_btn.setText("Start Capture")
            self.capture_logs_btn.setIcon(self.start_capture_icon)
            self.log_info("Log capture stopped.")

    # --- Node controls ---
    def start_nodes(self):
        payload = {
            "session_id": 99,
            "res_topic": "start_res",
            "operation": "START",
        }
        publish_message("cmd/proton-node-test/start", json.dumps(payload))

    def stop_nodes(self):
        payload = {
            "session_id": 99,
            "res_topic": "start_res",
            "operation": "STOP",
        }
        publish_message("cmd/proton-node-test/start", json.dumps(payload))

    def toggle_start_nodes(self):
        if not self.start_nodes_running:
            # Get duration from user input
            try:
                duration_text = self.duration_input.toPlainText().strip()
                duration_min = float(duration_text) if duration_text else 10.0
            except ValueError:
                duration_min = 10.0

            self.start_nodes()
            self.startNode_btn.setText("Stop Nodes")
            self.startNode_btn.setIcon(self.stopNode_icon)
            self.start_nodes_running = True
            self.log_info(f"Nodes started. Will stop automatically in {duration_min} minutes.")
            
            # Start timer to stop nodes automatically
            QTimer.singleShot(int(duration_min * 60 * 1000), self._auto_stop_nodes)
        else:
            self.stop_nodes()
            self.startNode_btn.setText("Start Nodes")
            self.startNode_btn.setIcon(self.startNode_icon)
            self.start_nodes_running = False
            self.log_info("Nodes stopped.")

    def _auto_stop_nodes(self):
        if self.start_nodes_running:
            self.toggle_start_nodes()
            self.log_info("Nodes stopped automatically by timer.")

    # --- CLA/NCLA ---
    def start_CLA(self):
        payload = {
            "session_id": 99,
            "res_topic": "control_res",
            "operation": "CLA",
        }

        selected_macs = [mac for checkbox, mac in self.device_checkboxes if checkbox.isChecked()]
        if not selected_macs:
            self.log_warn("[CLA] No devices selected.")
            return

        for dev in selected_macs:
            publish_message(f"cmd/proton-node-test/{dev}/control", json.dumps(payload))

    def stop_CLA(self):
        payload = {
            "session_id": 99,
            "res_topic": "control_res",
            "operation": "NCLA",
        }

        selected_macs = [mac for checkbox, mac in self.device_checkboxes if checkbox.isChecked()]
        if not selected_macs:
            self.log_warn("[NCLA] No devices selected.")
            return

        for dev in selected_macs:
            publish_message(f"cmd/proton-node-test/{dev}/control", json.dumps(payload))

    def toggle_start_CLA(self):
        if not self.start_CLA_running:
            self.start_CLA()
            self.device_status_button.setText("Stop CLA")
            self.start_CLA_running = True
        else:
            self.stop_CLA()
            self.device_status_button.setText("Start CLA")
            self.start_CLA_running = False

    # --- Firmware update ---
    def firmware_update(self):
        payload = {
            "session_id": self.Session_ID,
            "res_topic": "ota_dfu_res",
            "uri": "http://10.30.224.10:8080",
            "file": "net_update_cn.bin",
            "retries": 3,
            "install": "now"
        }

        selected_macs = [mac for checkbox, mac in self.device_checkboxes if checkbox.isChecked()]
        if not selected_macs:
            self.log_warn("[FOTA] No devices selected.")
            return

        for dev in selected_macs:
            publish_message(f"cmd/proton-node-test/{dev}/ota_dfu", json.dumps(payload))

        self.Session_ID += 1

    # --- Reboot ---
    def reboot_devices(self):
        payload = {
            "session_id": 99,
            "res_topic": "reboot_res",
            "delay": 0,
        }

        selected_macs = [mac for checkbox, mac in self.device_checkboxes if checkbox.isChecked()]
        if not selected_macs:
            self.log_warn("[REBOOT] No devices selected.")
            return

        for dev in selected_macs:
            publish_message(f"cmd/proton-node-test/{dev}/reboot", json.dumps(payload))

    # --- Device list ---
    def display_connected_devices(self):
        self.device_list.clear()
        self.device_checkboxes = []

        valid_mac_ids = {
            str(mac).strip()
            for mac in MAC_IDS
            if str(mac).strip().lower() not in {"", "node_id", "unknown", "none", "null"}
        }

        if not valid_mac_ids:
            self.log_warn("No connected devices found.")

        for index, mac in enumerate(sorted(valid_mac_ids), start=1):
            item_widget = QWidget()
            layout = QHBoxLayout()
            layout.setContentsMargins(10, 5, 10, 5)
            layout.setSpacing(20)

            checkbox = QCheckBox()
            index_label = QLabel(f"{index}.")
            mac_label = QLabel(str(mac))

            layout.addWidget(checkbox)
            layout.setAlignment(checkbox, Qt.AlignVCenter)
            layout.addSpacing(10)

            layout.addWidget(index_label)
            layout.setAlignment(index_label, Qt.AlignVCenter)
            layout.addSpacing(20)

            layout.addWidget(mac_label)
            layout.setAlignment(mac_label, Qt.AlignVCenter)

            layout.addStretch()
            item_widget.setLayout(layout)

            list_item = QListWidgetItem()
            list_item.setSizeHint(item_widget.sizeHint())

            self.device_list.addItem(list_item)
            self.device_list.setItemWidget(list_item, item_widget)

            self.device_checkboxes.append((checkbox, str(mac)))

    def animate_loading_text(self):
        self.loading_dots = (self.loading_dots + 1) % 4
        dots = "." * self.loading_dots
        self.get_devices_button.setText(f"Loading Devices{dots}")

    def finish_device_refresh(self):
        self.display_connected_devices()
        self.loading_timer.stop()
        self.get_devices_button.setText("Get Devices")
        self.get_devices_button.setEnabled(True)

    def update_device_list(self):
        self.get_devices_button.setEnabled(False)
        self.loading_dots = 0
        self.loading_timer.start(350)

        reset_mac_ids()

        payload = {
            "session_id": 99,
            "res_topic": "node_id_res",
            "operation": "GET_NODE_ID",
        }
        publish_message("cmd/proton-node-test/node_id", json.dumps(payload))

        QTimer.singleShot(FETCH_DEVICES_TIMEOUT_MS, self.finish_device_refresh)

    def toggle_all_checkboxes(self, state):
        checked = state == Qt.Checked
        for checkbox, _ in self.device_checkboxes:
            checkbox.setChecked(checked)

    # --- Custom MQTT publish ---
    def handle_json_input(self):
        raw_json = self.json_input.toPlainText().strip()
        topic = self.topic_input.toPlainText().strip()

        if not topic:
            self.log_error("No topic provided. Please enter a topic to publish to.")
            return

        try:
            parsed_json = json.loads(raw_json) if raw_json else {}
            pretty_json = json.dumps(parsed_json, indent=4)
            self.log_info(f"Publishing to topic: {topic}\n{pretty_json}")
            publish_message(topic, json.dumps(parsed_json))
        except json.JSONDecodeError as e:
            self.log_error(f"Invalid JSON: {e}")


def run_gui():
    app = QApplication([])
    window = MyWindow()
    window.show()
    return app.exec_()
