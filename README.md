# Wi-Fi and ESB Hybrid Wireless Network Codebase & Data

[![Zephyr RTOS](https://img.shields.io/badge/Zephyr-RTOS-green.svg)](https://zephyrproject.org/)
[![Nordic Semiconductor](https://img.shields.io/badge/Nordic-nRF5340-blue.svg)](https://www.nordicsemi.com/Products/nRF5340)

This repository contains the source code, configuration files, and field testing data for a robust and adaptive hybrid wireless network system. The system leverages both **Wi-Fi** (for long-range/high-bandwidth uplinks) and **Enhanced ShockBurst (ESB)** (for ultra-low power local communication). 

It is designed using the Nordic Semiconductor nRF5340 and nRF7002-based custom boards (e.g., Heliogen G5 Node), running on the Zephyr Real-Time Operating System (RTOS).

## System Architecture

The network architecture is hierarchical and split into two distinct roles to optimize power consumption and range:

### 1. Cluster Head (`cluster_head_multi_image_fota/`)
The Cluster Head acts as a bridge or gateway. It maintains a persistent or semi-persistent Wi-Fi connection to the cloud/WAN and coordinates local devices using Nordic's proprietary ESB protocol.
- **Wi-Fi Uplink**: Connects to the primary access point.
- **ESB Coordination**: Manages a local cluster of nodes.
- **Dual-Core IPC**: Utilizes Inter-Processor Communication (IPC) to separate the application logic (Application Core) from the networking stack (Network Core).

### 2. Cluster Node (`cluster_node_multi_image_fota/`)
The Cluster Nodes represent edge devices or sensors. They utilize ultra-low power ESB to connect back to the Cluster Head rather than using Wi-Fi directly, drastically improving battery life.
- **ESB Communication**: Aggregates and sends sensor data reliably to the Cluster Head.

### Features
- **Multi-Image Firmware Over-The-Air (FOTA)** support for updating both application and network core firmware securely.
- **IPC Handler (`src/ipc_handler.c`)** for efficient inter-core communication on the nRF5340.
- **Zephyr RTOS** integration using custom board overlays and `pm_static.yml` partitioning mapping.

## Field Testing Data

To validate the theoretical performance, extensive field testing has been conducted. The `data/` directory contains results across multiple network configurations (`Config 1`, `Config 2`, `Config 3`), evaluating the network under various real-world conditions.

### Dataset Structure
The dataset consists of `rtt_results.csv` files generated during the tests. Key metrics recorded include:

- **`Packet #`**: Sequence number of the transmitted packet.
- **`Device ID`**: Unique identifier (MAC address) of the testing node.
- **`TX Timestamp` / `RX Timestamp`**: Precise timing for packet departure and arrival.
- **`RTT Latency [ms]`**: End-to-end Round-Trip Time latency.
- **`Data Rate [Kbps]`**: Measured throughput.
- **`Response`**: Acknowledgment success metric.
- **`Latest IPC Msg`**: Hexadecimal trace of the inter-processor messaging.

These datasets are invaluable for statistical analysis regarding latency bounds, throughput limits, and ESB-to-Wi-Fi gateway reliability.

## Getting Started

### Prerequisites
- [nRF Connect SDK (NCS)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html) installed.
- `west` meta-tool for Zephyr.

### Building Firmware
The project relies on Zephyr's `west` build system. Ensure your environment is properly initialized for NCS.

**Build for Cluster Head:**
```console
west build -b heliogen_g5_node_nrf5340_cpuapp cluster_head_multi_image_fota
```

**Build for Cluster Node:**
```console
west build -b heliogen_g5_node_nrf5340_cpuapp cluster_node_multi_image_fota
```

### Flashing
Use the provided bash or batch scripts (`jflash.sh` / `jflash.bat`) located within each project directory, or alternatively use `west flash` if your runner is natively supported.
