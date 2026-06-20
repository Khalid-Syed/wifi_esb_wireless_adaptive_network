# PROTON Testbed GUI

A PyQt5-based user interface that acts as a **central server** for multiple wireless nodes communicating over **MQTT**.

## What this project does

- Provides a desktop GUI to:
  - Start/stop the Central Server
  - Start/stop nodes (via MQTT command topic)
  - Discover and list connected devices
  - Select devices and send control operations (CLA/NCLA), reboot, firmware update
  - Publish custom JSON payloads to any MQTT topic
  - Start/stop log capture for device telemetry topics

- Runs a Central Server backend that:
  - Serves UTC timestamps over TCP (time sync)
  - Subscribes to MQTT on `#` and processes inbound messages
  - Maintains a set of detected device IDs (MAC IDs)
  - Optionally captures CSV logs when log capture is enabled

## Project layout

```
PROTON-Testbed-gui/
  assets/                 # icons and other non-code assets
  scripts/                # packaging helpers (PyInstaller)
  src/
    proton_testbed_gui/   # main Python package
  proton_testbed.py       # thin launcher (kept for convenience/PyInstaller)
  requirements.txt
  pyproject.toml
```

## Install

Create a virtual environment (recommended) and install dependencies:

```bash
pip install -r requirements.txt
```

## Run

From the project root:

```bash
python proton_testbed.py
```

If you install the package (e.g. `pip install -e .`), you can also run:

```bash
python -m proton_testbed_gui
```

## MQTT broker (Mosquitto)

The Central Server expects an MQTT broker on `127.0.0.1:1883`.

- On **Windows**, the code attempts to start/stop the `mosquitto` service using `net start mosquitto` / `net stop mosquitto`.
- On **non-Windows** platforms, you must ensure the broker is already running.

## Build an EXE (PyInstaller)

See `scripts/make_exe.txt` and `scripts/proton_testbed.spec`.

Example:

```bash
pyinstaller --onefile --noconsole --paths=src --icon=assets/image.ico proton_testbed.py
```

## Author

Author: Md Sadman Siraj  
Email: msiraj13@asu.edu  
Date: 2026-02-02

## Credits

Original implementation and feature work were based on prior project code authored by Mohamed Umar.
