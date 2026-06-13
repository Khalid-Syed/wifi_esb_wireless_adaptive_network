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

### Prerequisites

- [nRF Connect SDK (NCS)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html) installed and sourced via `west init` / `west update`
- `west` meta-tool available in your PATH
- J-Link driver installed (for `west flash` / `nrfjprog`)

> **Run all `west build` commands from the NCS workspace root** (the directory containing the `west.yml` manifest), not from inside this folder.

### Debug Build

```console
west build -b heliogen_g5_node_nrf5340_cpuapp cluster_head_multi_image_fota
```

### Release Build (MCUboot FOTA + Watchdog enabled)

```console
west build -b heliogen_g5_node_nrf5340_cpuapp cluster_head_multi_image_fota -- \
  -DOVERLAY_CONFIG=overlay_release.conf
```

### Release Build + Wi-Fi Credentials

To embed Wi-Fi SSID and passphrase at compile time:

```console
west build -b heliogen_g5_node_nrf5340_cpuapp cluster_head_multi_image_fota -- \
  -DOVERLAY_CONFIG="overlay_release.conf;overlay-wifi-config.conf"
```

### Rebuild Without Reconfiguring

After the first build, you can rebuild faster with:

```console
west build
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

### Option 3 — J-Flash (SEGGER GUI / Scripted)

The jflash scripts in the Cluster Node directory serve as a reference. To adapt them for the Cluster Head, point them at:

- **Binary:** `build/zephyr/merged.hex`
- **J-Flash project:** your board's `.jflash` project file (configured for nRF5340)

**Linux / macOS:**
```bash
"/Applications/SEGGER/JLink_V758d/JFlash.app/Contents/MacOS/JFlashExe" \
  -openprj nordic.jflash \
  -open build/zephyr/merged.hex \
  -auto -min -exit
```

**Windows:**
```bat
"C:\Program Files\SEGGER\JLink\JFlash.exe" ^
  -openprjnordic.jflash ^
  -openbuild\zephyr\merged.hex ^
  -auto -min -exit
```

---

## Runtime Shell Commands

The firmware exposes a full Zephyr Shell over both **UART** and **SEGGER RTT**. Key command groups:

| Shell Command | Description |
|---|---|
| `wifi scan` | Scan for nearby access points |
| `wifi connect <SSID> <passphrase>` | Connect to a Wi-Fi network |
| `wifi status` | Show current Wi-Fi connection state |
| `mqtt <cmd>` | MQTT connection management |
| `cfg <key> <value>` | Read/write runtime configuration |
| `fs <cmd>` | LittleFS file system operations |
| `reboot` | Trigger a system reboot |
| `version` | Print firmware version string |
| `boot_count` | Display boot cycle counter |

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
