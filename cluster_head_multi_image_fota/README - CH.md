# Cluster Head Firmware — `cluster_head_multi_image_fota`

[![Zephyr RTOS](https://img.shields.io/badge/Zephyr-RTOS-green.svg)](https://zephyrproject.org/)
[![Nordic Semiconductor](https://img.shields.io/badge/Nordic-nRF5340-blue.svg)](https://www.nordicsemi.com/Products/nRF5340)
[![License](https://img.shields.io/badge/License-LicenseRef--Nordic--5--Clause-orange.svg)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/licenses.html)

## Overview

This directory contains the **Application Core** firmware for the **Cluster Head** node in the Wi-Fi / ESB hybrid wireless network system. The Cluster Head acts as the central gateway in the network hierarchy — bridging a local ESB wireless cluster of sensor nodes to the cloud via Wi-Fi and MQTT.

It targets the **Heliogen G5 Node** custom board, built on the **Nordic Semiconductor nRF5340** SoC paired with the **nRF7002** Wi-Fi companion chip, and runs on the **Zephyr Real-Time Operating System (RTOS)** under the **nRF Connect SDK (NCS)**.

---

## Role in the Network

```
[Cluster Nodes] ──ESB──► [Cluster Head (this firmware)] ──Wi-Fi/MQTT──► [Cloud / Server]
```

The Cluster Head performs two primary functions simultaneously:

| Function | Interface | Description |
|---|---|---|
| **Local Coordination** | ESB (via Net Core) | Receives sensor payloads from Cluster Nodes over Enhanced ShockBurst |
| **Cloud Uplink** | Wi-Fi (nRF7002) | Forwards aggregated data to an MQTT broker over TCP/IP |

---

## Architecture

### Dual-Core Split (nRF5340)

The nRF5340 is a dual-core SoC. Firmware responsibilities are split as follows:

| Core | Responsibility |
|---|---|
| **Application Core** (`cpuapp`) | Wi-Fi stack (WPA Supplicant), MQTT transport, IPC service, application logic, FOTA management |
| **Network Core** (`cpunet`) | ESB protocol driver, radio management |

The two cores communicate via the **IPC service** using the **ICMsg backend** over hardware MBOX. The shared message format is defined in `main.c`:

```c
struct esb_ipc_msg {
    uint8_t type;       // TX=0x01, RX=0x02, NODE_ID=0x03
    uint8_t length;
    uint8_t data[32];
} __packed;
```

### IPC Message Flow

1. An ESB packet arrives at the Network Core from a Cluster Node.
2. The Net Core wraps it as an `ESB_IPC_MSG_TYPE_RX` IPC message and sends it to the App Core.
3. The App Core's `recv_cb()` receives the message and immediately calls `process_immediate_esb_rx()`.
4. The ESB payload is hex-encoded and published to the cloud via MQTT using `publish_response_external()`.

On the downlink path, the App Core constructs an `ESB_IPC_MSG_TYPE_TX` message and calls `ipc_send_and_wait()`, which blocks until a response is received from the Net Core.

---

## Source Code Structure (`src/`)

```
src/
├── main.c                      # Entry point: IPC init, boot sequence, IPC Tx/Rx dispatch
├── ipc_handler.c / .h          # IPC helper utilities
├── bsp/                        # Board Support Package (GPIO, SPI Flash, SAADC, NTC, PWM, timers)
│   └── id/                     # Device ID (MAC-based hostname) utilities
├── cfg/                        # Runtime configuration (JSON-backed, LittleFS-stored)
├── data/
│   ├── json/                   # JSON serialization for all MQTT message types
│   └── mqtt/                   # MQTT topic handlers (RTT, control, status, OTA, node ID, etc.)
├── events/                     # App Event Manager events (Wi-Fi, OTA/DFU, transport, power)
├── hal/                        # Hardware Abstraction Layer
├── services/
│   ├── http_update/            # HTTP-based FOTA download client
│   └── publisher/              # MQTT publish table and per-topic publishers
├── shell_cmds/                 # Zephyr Shell commands (MQTT, Wi-Fi, MPPT, reboot, stats, etc.)
├── storage/                    # LittleFS storage, counters, and shell interface
├── sys/                        # System utilities (boot count, reset cause, watchdog, version)
├── threads/                    # RTOS threads (manager, Wi-Fi, power, time sync, status)
├── transport/                  # Low-level MQTT transport layer
└── utils/                      # String utilities, random number, jitter, nearest-element lookup
```

---

## Key Configuration Files

| File | Purpose |
|---|---|
| `prj.conf` | Base Kconfig configuration (IPC, Wi-Fi, sockets, flash, LittleFS, shell, RTT logging) |
| `overlay_release.conf` | Release profile: enables MCUboot FOTA, watchdog, size optimization, PCD for Net Core updates |
| `overlay_debug.conf` | Debug profile overrides |
| `overlay-wifi-config.conf` | Wi-Fi SSID / passphrase and connection settings |
| `overlay-scan-only.conf` | Wi-Fi scan-only mode (no association) |
| `overlay-zperf.conf` | Zephyr performance benchmark overlay |
| `pm_static_heliogen_g5_node_nrf5340_cpuapp.yml` | Static Partition Manager layout (internal + external flash regions for MCUboot + LittleFS) |
| `sample.yaml` | Board compatibility descriptor |
| `net_ipc/` | Network Core child image (ESB radio firmware) |

---

## Pre-built Binaries

The following pre-built binaries are included for convenience:

| Binary | Description |
|---|---|
| `app_update_ch.bin` | Application Core OTA update image (Cluster Head) |
| `net_update_ch_A.bin` – `_E.bin` | Network Core OTA update images (channel variants A–E) |

These binaries are used for field FOTA updates and do not require a local build.

---

## Building

### How the Dual-Core Build Works

The nRF5340 has two independent cores — both must be programmed:

| Core | Board target suffix | Firmware | Source |
|---|---|---|---|
| **Application Core** | `_cpuapp` | Wi-Fi, MQTT, IPC service, application logic | `src/` |
| **Network Core** | `_cpunet` | ESB radio modem (PTX/PRX mode) | `net_ipc/src/` |

The Net Core firmware (`net_ipc/`) is a **child image** — it is compiled automatically as part of the App Core `west build`. You do **not** build the Net Core separately. The build system produces a single `merged.hex` that contains both cores, as well as individual update binaries for FOTA.

### Prerequisites

- [nRF Connect SDK (NCS)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html) installed and sourced via `west init` / `west update`
- `west` meta-tool available in your PATH
- J-Link driver installed (for `west flash` / `nrfjprog`)

> **Run all `west build` commands from the NCS workspace root** (the directory containing the `west.yml` manifest), not from inside this folder.

### Standard Build Command

This is the full build command used for the Cluster Head. Run it from anywhere — the `--build-dir` and source path are absolute:

```console
west build \
  --build-dir <workspace-path>/cluster_head_multi_image_fota/build-node_1 \
  <workspace-path>/cluster_head_multi_image_fota \
  --pristine \
  --board heliogen_g5_node_nrf5340_cpuapp \
  --no-sysbuild \
  -- \
  -DNCS_TOOLCHAIN_VERSION="NONE" \
  -DCONFIG_SIZE_OPTIMIZATIONS=y \
  -DCONFIG_DEBUG_THREAD_INFO=y \
  -DCONF_FILE="prj.conf" \
  -DEXTRA_CONF_FILE="overlay-wifi-config.conf;overlay-logging.conf;overlay_release.conf;conf/version.conf" \
  -DDTC_OVERLAY_FILE=boards/heliogen_g5_node_nrf5340_cpuapp.overlay \
  -DBOARD_ROOT="<workspace-path>/cluster_head_multi_image_fota"
```

**On Windows (single line):**
```console
west build --build-dir <workspace-path>/cluster_head_multi_image_fota/build-node_1 <workspace-path>/cluster_head_multi_image_fota --pristine --board heliogen_g5_node_nrf5340_cpuapp --no-sysbuild -- -DNCS_TOOLCHAIN_VERSION="NONE" -DCONFIG_SIZE_OPTIMIZATIONS=y -DCONFIG_DEBUG_THREAD_INFO=y -DCONF_FILE="prj.conf" -DEXTRA_CONF_FILE="overlay-wifi-config.conf;overlay-logging.conf;overlay_release.conf;conf/version.conf" -DDTC_OVERLAY_FILE=boards/heliogen_g5_node_nrf5340_cpuapp.overlay -DBOARD_ROOT="<workspace-path>/cluster_head_multi_image_fota"
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
| `CONFIG_DEBUG_THREAD_INFO` | `y` | Embeds thread metadata for SEGGER SystemView / Ozone debugging |
| `CONF_FILE` | `prj.conf` | Base Kconfig configuration |
| `EXTRA_CONF_FILE` | See below | Layered overlays applied on top of `prj.conf` |
| `DTC_OVERLAY_FILE` | `boards/...overlay` | Board-specific devicetree overlay (pin assignments, peripherals) |
| `BOARD_ROOT` | Project directory | Tells Zephyr where to find the custom board definition |

### Overlay Stack (`EXTRA_CONF_FILE`)

Overlays are applied **in order**, with later files taking precedence:

| Overlay | Purpose |
|---|---|
| `overlay-wifi-config.conf` | Wi-Fi SSID, passphrase, and connection settings |
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

- **App Core update**: `app_update_ch.bin` (downloaded over Wi-Fi via HTTP)
- **Net Core update**: `net_update_ch_*.bin` (transferred to Net Core via PCD — Peripheral CPU Debug interface)

The firmware auto-confirms the running image on boot via `boot_write_img_confirmed_multi()` to prevent MCUboot rollback.

---

## Dependencies

- [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)
- [Zephyr RTOS](https://zephyrproject.org/)
- `modules/lib/hostap` (WPA Supplicant / Wi-Fi stack)
- `modules/mbedtls` (TLS for HTTPS FOTA)
- `sdk-nrfxlib` → `nrf_security`
