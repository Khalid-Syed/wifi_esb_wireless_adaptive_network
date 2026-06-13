# Cluster Node Firmware — `cluster_node_multi_image_fota`

[![Zephyr RTOS](https://img.shields.io/badge/Zephyr-RTOS-green.svg)](https://zephyrproject.org/)
[![Nordic Semiconductor](https://img.shields.io/badge/Nordic-nRF5340-blue.svg)](https://www.nordicsemi.com/Products/nRF5340)
[![License](https://img.shields.io/badge/License-LicenseRef--Nordic--5--Clause-orange.svg)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/licenses.html)

## Overview

This directory contains the **Application Core** firmware for the **Cluster Node** in the Wi-Fi / ESB hybrid wireless network system. Cluster Nodes are the sensing edge devices of the network — they collect data and communicate exclusively over **Enhanced ShockBurst (ESB)** to a Cluster Head, avoiding the power overhead of Wi-Fi entirely.

It targets the **Heliogen G5 Node** custom board, built on the **Nordic Semiconductor nRF5340** SoC, running on the **Zephyr Real-Time Operating System (RTOS)** under the **nRF Connect SDK (NCS)**.

---

## Role in the Network

```
[Sensors / Peripherals] ──► [Cluster Node (this firmware)] ──ESB──► [Cluster Head] ──Wi-Fi──► [Cloud]
```

The Cluster Node is an ultra-low-power edge device. Rather than connecting directly to Wi-Fi, it relies on ESB to report its data to the nearest Cluster Head, which handles the cloud uplink:

| Function | Interface | Description |
|---|---|---|
| **Sensor Data Collection** | SPI, ADC, GPIO | Interfaces with onboard and external sensors |
| **Local Radio Link** | ESB (via Net Core) | Transmits data to and receives commands from the Cluster Head |
| **Battery Monitoring** | SAADC | Measures battery voltage and reports it to the Net Core on demand |

---

## Architecture

### Dual-Core Split (nRF5340)

The nRF5340 is a dual-core SoC. Firmware responsibilities are split as follows:

| Core | Responsibility |
|---|---|
| **Application Core** (`cpuapp`) | Application logic, sensor management, IPC service, FOTA management, configuration |
| **Network Core** (`cpunet`) | ESB protocol driver, radio management |

The two cores communicate via the **IPC service** using the **ICMsg backend** over hardware MBOX. The shared message format is:

```c
struct esb_ipc_msg {
    uint8_t type;       // TX=0x01, RX=0x02, NODE_ID=0x03, BATT=0x04, REQ_BATT=0x05
    uint8_t length;
    uint8_t data[32];
} __packed;
```

#### IPC Message Types (Cluster Node additions vs. Cluster Head)

| Type | Value | Direction | Description |
|---|---|---|---|
| `ESB_IPC_MSG_TYPE_TX` | `0x01` | App → Net | Send ESB payload to Cluster Head |
| `ESB_IPC_MSG_TYPE_RX` | `0x02` | Net → App | ESB payload received from Cluster Head |
| `ESB_IPC_MSG_TYPE_NODE_ID` | `0x03` | App → Net | Send device hostname to Net Core |
| `ESB_IPC_MSG_TYPE_BATT` | `0x04` | App → Net | Push battery voltage reading to Net Core |
| `ESB_IPC_MSG_TYPE_REQ_BATT` | `0x05` | Net → App | Net Core requests a fresh battery voltage |

### Battery Voltage Reporting

The Cluster Node includes dedicated battery telemetry logic. When the Net Core sends an `ESB_IPC_MSG_TYPE_REQ_BATT` message, the App Core reads the latest averaged battery voltage from `power_thread.c` (`battery_mv_avg`) and responds via `ipc_send_battery()`:

```c
int ipc_send_battery(int32_t battery_mv);
```

This allows battery state to be piggybacked onto ESB transmissions without the App Core needing to manage radio timing.

---

## Source Code Structure (`src/`)

```
src/
├── main.c                      # Entry point: IPC init, boot sequence, battery Tx/Rx dispatch
├── ipc_handler.c / .h          # IPC helper utilities
├── bsp/                        # Board Support Package (GPIO, SPI Flash, SAADC, NTC, PWM, timers)
│   └── id/                     # Device ID (MAC-based hostname) utilities
├── cfg/                        # Runtime configuration (JSON-backed, LittleFS-stored)
├── data/
│   ├── json/                   # JSON serialization for status and message types
│   └── mqtt/                   # (Stub) MQTT message types (status only when Wi-Fi disabled)
├── events/                     # App Event Manager events (Wi-Fi, OTA/DFU, transport, power)
├── hal/                        # Hardware Abstraction Layer
├── services/
│   └── http_update/            # HTTP-based FOTA download client (release builds)
├── shell_cmds/                 # Zephyr Shell commands (version, reboot, boot count, stats, etc.)
├── storage/                    # LittleFS storage, counters, and shell interface
├── sys/                        # System utilities (boot count, reset cause, watchdog, version)
├── threads/                    # RTOS threads (manager, power, time sync, status)
├── transport/                  # Transport layer (conditionally compiled with MQTT)
└── utils/                      # String utilities, random number, jitter, nearest-element lookup
```

> **Note:** Unlike the Cluster Head, the Cluster Node does **not** compile the Wi-Fi thread or MQTT transport stack by default. The `CONFIG_WIFI` and `CONFIG_MQTT_HELPER_HELIOGEN` flags control their inclusion.

---

## Key Configuration Files

| File | Purpose |
|---|---|
| `prj.conf` | Base Kconfig configuration (IPC, flash, LittleFS, shell, RTT logging, SAADC, timers) |
| `overlay_release.conf` | Release profile: enables MCUboot FOTA, watchdog, size optimization, PCD for Net Core updates |
| `overlay_debug.conf` | Debug profile overrides |
| `overlay-logging.conf` | Extended logging configuration overlay |
| `overlay-wifi-config.conf` | Wi-Fi credentials (for testing/debugging with Wi-Fi enabled) |
| `overlay-ipv6-only.conf` | IPv6-only network mode overlay |
| `overlay-scan-only.conf` | Wi-Fi scan-only mode overlay |
| `overlay-zperf.conf` | Zephyr performance benchmark overlay |
| `pm_static_heliogen_g5_node_nrf5340_cpuapp.yml` | Static Partition Manager layout (internal + external flash regions) |
| `CMakePresets.json` | CMake build presets for common configurations |
| `jflash.sh` / `jflash.bat` | J-Link flash scripts for Linux/macOS and Windows |
| `sample.yaml` | Board compatibility descriptor |
| `net_ipc/` | Network Core child image (ESB radio firmware) |

---

## Pre-built Binaries

| Binary | Description |
|---|---|
| `app_update_cn.bin` | Application Core OTA update image (Cluster Node) |
| `net_update.bin` | Network Core OTA update image |

These binaries are used for field FOTA updates and do not require a local build.

---

## Building

### Prerequisites

- [nRF Connect SDK (NCS)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html) installed and sourced via `west init` / `west update`
- `west` meta-tool available in your PATH
- J-Link driver installed (for `west flash` / `nrfjprog`)

> **Run all `west build` commands from the NCS workspace root** (the directory containing the `west.yml` manifest), not from inside this folder.

### Debug Build

```console
west build -b heliogen_g5_node_nrf5340_cpuapp cluster_node_multi_image_fota
```

### Release Build (MCUboot FOTA + Watchdog enabled)

```console
west build -b heliogen_g5_node_nrf5340_cpuapp cluster_node_multi_image_fota -- \
  -DOVERLAY_CONFIG=overlay_release.conf
```

### Rebuild Without Reconfiguring

After the first build, you can rebuild faster with:

```console
west build
```

### Using CMake Presets

If a matching preset is defined in `CMakePresets.json`:

```console
west build --cmake-only -- --preset <preset-name>
```

### Build Outputs

All build artifacts land in `build/zephyr/`:

| File | Description |
|---|---|
| `build/zephyr/merged.hex` | Full flash image (MCUboot + App Core) — use this for initial programming |
| `build/zephyr/app_update.bin` | App Core OTA update binary (for FOTA) |
| `build/zephyr/net_core_app_update.bin` | Net Core OTA update binary (release builds only) |

---

## Flashing

### Option 1 — `west flash` (Recommended)

Flashes the complete `merged.hex` (MCUboot + application) in one step:

```console
west flash
```

To target a specific J-Link serial number when multiple boards are connected:

```console
west flash --snr <J-Link-serial-number>
```

### Option 2 — `nrfjprog` (Manual / Scripted)

Use `nrfjprog` directly for finer control. The nRF5340 has two cores — flash them in order:

#### Step 1 — Erase the device

```console
nrfjprog --eraseall -f NRF53 --coprocessor CP_APPLICATION
```

#### Step 2 — Flash the merged image (App Core)

```console
nrfjprog --program build/zephyr/merged.hex --verify -f NRF53 --coprocessor CP_APPLICATION
```

#### Step 3 — Reset and run

```console
nrfjprog --reset -f NRF53
```

#### Targeting a specific board by serial number

```console
nrfjprog --program build/zephyr/merged.hex --verify -f NRF53 \
  --coprocessor CP_APPLICATION --snr <J-Link-serial-number>
nrfjprog --reset -f NRF53 --snr <J-Link-serial-number>
```

> **Note:** On the nRF5340, the Network Core firmware is embedded inside the App Core image when using multi-image builds. You do **not** need to flash the Net Core separately on a fresh board — `merged.hex` handles both.

### Option 3 — J-Flash Scripts (SEGGER)

This directory includes pre-configured J-Flash scripts that flash `build/zephyr/merged.hex` using a J-Flash project file (`nordic.jflash`).

**Linux / macOS** (`jflash.sh`):
```bash
bash jflash.sh
```

Or run the underlying command directly:
```bash
"/Applications/SEGGER/JLink_V758d/JFlash.app/Contents/MacOS/JFlashExe" \
  -openprj jflash-5340.jflash \
  -open build_wifi/zephyr/merged.hex \
  -auto -jflashlog jflash_log.log -min -exit
```

**Windows** (`jflash.bat`):
```bat
jflash.bat
```

Or run the underlying command directly:
```bat
"C:\Program Files\SEGGER\JLink\JFlash.exe" ^
  -openprjnordic.jflash ^
  -openbuild\zephyr\merged.hex ^
  -auto -jflashlog jflash_log.log -min -exit
```

> The batch script uses `nordic.jflash` as the project file and targets `build\zephyr\merged.hex`. Update these paths if your build directory or J-Flash project file name differs.

---

## Runtime Shell Commands

The firmware exposes a Zephyr Shell over both **UART** and **SEGGER RTT**. Key command groups:

| Shell Command | Description |
|---|---|
| `cfg <key> <value>` | Read/write runtime configuration |
| `fs <cmd>` | LittleFS file system operations |
| `reboot` | Trigger a system reboot |
| `version` | Print firmware version string |
| `boot_count` | Display boot cycle counter |
| `stats` | Display system statistics |
| `eol` | End-of-line test result management |

---

## FOTA (Firmware Over-The-Air)

Multi-image FOTA is supported for **both cores** via MCUboot:

- **App Core update**: `app_update_cn.bin`
- **Net Core update**: `net_update.bin` (transferred to Net Core via PCD — Peripheral CPU Debug interface)

The firmware auto-confirms the running image on boot via `boot_write_img_confirmed_multi()` to prevent MCUboot rollback.

---

## Differences vs. Cluster Head

| Feature | Cluster Head | Cluster Node |
|---|---|---|
| **Wi-Fi uplink** | ✅ Active | ❌ Not used |
| **MQTT publishing** | ✅ Full stack | ❌ Not used |
| **Battery telemetry IPC** | ❌ | ✅ `ipc_send_battery()` |
| **ESB-to-MQTT forwarding** | ✅ `process_immediate_esb_rx()` | ❌ |
| **J-Flash scripts** | ❌ | ✅ `jflash.sh` / `jflash.bat` |
| **CMake presets** | ❌ | ✅ `CMakePresets.json` |
| **Hostname** | `g5node-<MAC>` | `g5node-<MAC>` |

---

## Dependencies

- [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)
- [Zephyr RTOS](https://zephyrproject.org/)
- `sdk-nrfxlib` → `nrf_security`
