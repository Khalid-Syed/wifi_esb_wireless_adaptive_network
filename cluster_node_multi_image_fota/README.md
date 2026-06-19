# Cluster Node Firmware — `cluster_node_multi_image_fota`

[![Zephyr RTOS](https://img.shields.io/badge/Zephyr-RTOS-green.svg)](https://zephyrproject.org/)
[![Nordic Semiconductor](https://img.shields.io/badge/Nordic-nRF5340-blue.svg)](https://www.nordicsemi.com/Products/nRF5340)
[![License](https://img.shields.io/badge/License-LicenseRef--Nordic--5--Clause-orange.svg)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/licenses.html)

## Overview

This directory contains the **Application Core and the Network Core** firmware for the **Cluster Node** in the Wi-Fi / ESB hybrid wireless network system. Cluster Nodes are the sensing edge devices of the network — they collect data and communicate exclusively over **Enhanced ShockBurst (ESB)** to a Cluster Head, avoiding the power overhead of Wi-Fi entirely. 

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
| `pm_static_heliogen_g5_node_nrf5340_cpuapp.yml` | Static Partition Manager layout (internal + external flash regions)  |
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

### How the Dual-Core Build Works

The nRF5340 has two independent cores — both must be programmed:

| Core | Board target suffix | Firmware | Source |
|---|---|---|---|
| **Application Core** | `_cpuapp` | Sensor logic, IPC service, FOTA management, configuration | `src/` |
| **Network Core** | `_cpunet` | ESB radio modem (PRX mode), battery telemetry relay | `net_ipc/src/` |

The Net Core firmware (`net_ipc/`) is a **child image** — it is compiled automatically as part of the App Core `west build`. You do **not** build the Net Core separately. The build system produces a single `merged.hex` containing both cores, plus individual update binaries for FOTA.

### Prerequisites

- [nRF Connect SDK (NCS)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html) installed and sourced via `west init` / `west update`
- `west` meta-tool available in your PATH
- J-Link driver installed (for `west flash` / `nrfjprog`)

> **Run all `west build` commands from the NCS workspace root** (the directory containing the `west.yml` manifest), not from inside this folder.

### Standard Build Command

This is the full build command used for the Cluster Node. Run it from anywhere — the `--build-dir` and source path are absolute:

```console
west build \
  --build-dir <workspace-path>/cluster_node_multi_image_fota/build-node_1 \
  <workspace-path>/cluster_node_multi_image_fota \
  --pristine \
  --board heliogen_g5_node_nrf5340_cpuapp \
  --no-sysbuild \
  -- \
  -DNCS_TOOLCHAIN_VERSION="NONE" \
  -DCONFIG_SIZE_OPTIMIZATIONS=y \
  -DCONF_FILE="prj.conf" \
  -DEXTRA_CONF_FILE="overlay-wifi-config.conf;overlay-logging.conf;overlay_release.conf;conf/version.conf" \
  -DDTC_OVERLAY_FILE=boards/heliogen_g5_node_nrf5340_cpuapp.overlay \
  -DBOARD_ROOT="<workspace-path>/cluster_node_multi_image_fota"
```

**On Windows (single line):**
```console
west build --build-dir <workspace-path>/cluster_node_multi_image_fota/build-node_1 <workspace-path>/cluster_node_multi_image_fota --pristine --board heliogen_g5_node_nrf5340_cpuapp --no-sysbuild -- -DNCS_TOOLCHAIN_VERSION="NONE" -DCONFIG_SIZE_OPTIMIZATIONS=y -DCONF_FILE="prj.conf" -DEXTRA_CONF_FILE="overlay-wifi-config.conf;overlay-logging.conf;overlay_release.conf;conf/version.conf" -DDTC_OVERLAY_FILE=boards/heliogen_g5_node_nrf5340_cpuapp.overlay -DBOARD_ROOT="<workspace-path>/cluster_node_multi_image_fota"
```

### Key Flags Explained

| Flag | Value | Purpose |
|---|---|---|
| `--build-dir` | `build-node_1` | Output directory for build artifacts |
| `--pristine` | — | Clears the build directory before building (equivalent to a clean build) |
| `--board` | `heliogen_g5_node_nrf5340_cpuapp` | Targets the App Core of the Heliogen G5 custom board |
| `--no-sysbuild` | — | Disables the sysbuild multi-image system; child images (Net Core) are handled via CMake directly |
| `NCS_TOOLCHAIN_VERSION` | `NONE` | Uses the toolchain already on PATH rather than an NCS-managed version |
| `CONFIG_SIZE_OPTIMIZATIONS` | `y` | Enables `-Os` compiler flag to minimise flash usage |
| `CONF_FILE` | `prj.conf` | Base Kconfig configuration |
| `EXTRA_CONF_FILE` | See below | Layered overlays applied on top of `prj.conf` |
| `DTC_OVERLAY_FILE` | `boards/...overlay` | Board-specific devicetree overlay (pin assignments, peripherals) |
| `BOARD_ROOT` | Project directory | Tells Zephyr where to find the custom board definition |

> **Note:** Unlike the Cluster Head build, `CONFIG_DEBUG_THREAD_INFO` is not set here.

### Overlay Stack (`EXTRA_CONF_FILE`)

Overlays are applied **in order**, with later files taking precedence:

| Overlay | Purpose |
|---|---|
| `overlay-wifi-config.conf` | Wi-Fi credentials (used during debug/testing) |
| `overlay-logging.conf` | Extended log levels and RTT/UART logging configuration |
| `overlay_release.conf` | MCUboot FOTA, watchdog, PCD (Net Core update), external flash secondary slot |
| `conf/version.conf` | Firmware version string injected at build time |


### Build Outputs

All build artifacts land in `build/zephyr/`:

| File | Core | Description |
|---|---|---|
| `build/zephyr/merged.hex` | Both | Complete flash image (MCUboot + App Core + Net Core) — use this for initial board programming |
| `build/zephyr/zephyr.hex` | App Core only | App Core image without MCUboot |
| `build/zephyr/app_update.bin` | App Core | Signed OTA update binary for FOTA (App Core only) |
| `build/zephyr/net_core_app_update.bin` | Net Core | Signed OTA update binary for FOTA (Net Core only, release builds) |
| `build/zephyr/hci_ipc/zephyr/zephyr.hex` | Net Core | Net Core image (child image build output) |

---

## Flashing

### Option 1 — `west flash` (Recommended for initial programming)

`west flash` flashes `merged.hex` — a combined image containing **both** the App Core and the Net Core — in a single command:

```console
west flash
```

To target a specific board when multiple J-Links are connected:

```console
west flash --snr <J-Link-serial-number>
```

---

### Option 2 — `nrfjprog` (Flash each core individually)

Use this method when you need to update only one core, or for scripted/CI workflows.

#### Flash the Application Core

```console
# Erase App Core flash
nrfjprog --eraseall -f NRF53 --coprocessor CP_APPLICATION

# Program App Core (merged.hex includes Net Core image — use for fresh boards)
nrfjprog --program build/zephyr/merged.hex --verify -f NRF53 --coprocessor CP_APPLICATION

# Reset to run
nrfjprog --reset -f NRF53
```

#### Flash the Network Core only (standalone update)

If you only need to update the Net Core without touching the App Core:

```console
# Erase Net Core flash
nrfjprog --eraseall -f NRF53 --coprocessor CP_NETWORK

# Program Net Core child image directly
nrfjprog --program build/zephyr/hci_ipc/zephyr/zephyr.hex \
  --verify -f NRF53 --coprocessor CP_NETWORK

# Reset
nrfjprog --reset -f NRF53
```

> **Note:** The Net Core child image hex is located at `build/zephyr/hci_ipc/zephyr/zephyr.hex` after a successful multi-image build. Adjust the path if your Zephyr version names the subdirectory differently (sometimes `net_ipc`).

#### Full erase + flash both cores (clean slate)

```console
# Erase both cores
nrfjprog --eraseall -f NRF53 --coprocessor CP_APPLICATION
nrfjprog --eraseall -f NRF53 --coprocessor CP_NETWORK

# Flash App Core (merged.hex bundles the Net Core image too)
nrfjprog --program build/zephyr/merged.hex --verify -f NRF53 --coprocessor CP_APPLICATION

# Flash Net Core explicitly
nrfjprog --program build/zephyr/hci_ipc/zephyr/zephyr.hex \
  --verify -f NRF53 --coprocessor CP_NETWORK

# Reset
nrfjprog --reset -f NRF53
```

#### Targeting a specific board by serial number

Append `--snr <J-Link-serial-number>` to any `nrfjprog` command:

```console
nrfjprog --program build/zephyr/merged.hex --verify -f NRF53 \
  --coprocessor CP_APPLICATION --snr <J-Link-serial-number>
nrfjprog --reset -f NRF53 --snr <J-Link-serial-number>
```

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
| **Hostname** | `g5node-<MAC>` | `g5node-<MAC>` |

---

## Dependencies

- [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)
- [Zephyr RTOS](https://zephyrproject.org/)
- `sdk-nrfxlib` → `nrf_security`
