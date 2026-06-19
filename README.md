# Wi-Fi and ESB Hybrid Wireless Network Codebase & Data

[![Zephyr RTOS](https://img.shields.io/badge/Zephyr-RTOS-green.svg)](https://zephyrproject.org/)
[![Nordic Semiconductor](https://img.shields.io/badge/Nordic-nRF5340-blue.svg)](https://www.nordicsemi.com/Products/nRF5340)

This repository contains the source code, configuration files, and field testing data for a robust and adaptive hybrid wireless network system.

This hybrid network architecture delivers an end-to-end, low-latency, and energy-efficient communication framework that drastically improves scalability and coverage without the need for excessive, cost-prohibitive infrastructure deployment by leveraging both **Wi-Fi** (for long-range/high-bandwidth uplinks) and **Enhanced ShockBurst (ESB)** (for ultra-low power local communication). Originally developed to resolve critical communication and networking bottlenecks in counter-UAV systems, this architecture seamlessly adapts to _wide-area deployments featuring high-density, stationary transmitters—including industrial telemetry, solar power plants, precision agriculture, and large-scale infrastructure monitoring_.

<img width="1070" height="436" alt="DataFlowmp4" src="https://github.com/user-attachments/assets/43c8e8c4-ea1e-4010-b56d-45a5c9dc7e0a" />


*Key Architectural Advantages:*
- **Cost-Effective Scalability (Access & Backhaul Split):** Traditional wireless deployments scale by adding expensive Wi-Fi Access Points (APs) to match device density. This architecture bypasses that constraint by creating a decoupled access and backhaul network. By utilizing Enhanced ShockBurst (ESB) exclusively for the local access layer, multiple of edge nodes can communicate with a single gateway/central server without relying on multiple APs. This eliminates infrastructure overhead and significantly reduces deployment costs.

- **Ultra-Low Latency (<200 ms end to end):** Because ESB is a lightweight, purely radio-based protocol, it avoids the heavy connection handshakes, beaconing, and packet overhead inherent to standard Wi-Fi. This streamlined, bare-metal radio communication ensures rapid, deterministic data delivery from the edge to the gateway.

- **High Energy Efficiency:** The ultra-low protocol overhead of ESB translates directly to shorter radio transmission times. Edge nodes spend minimal power on packet overhead and can aggressively utilize deep-sleep cycles, drastically maximizing battery life compared to power-hungry Wi-Fi edge alternatives.

The firmware in this repository is designed for the Nordic Semiconductor nRF5340 and nRF7002-based boards (e.g., Heliogen G5 Node), running on the Zephyr Real-Time Operating System (RTOS). However, the underlying architectural concepts and the majority of the firmware are platform-agnostic and can be adapted to other hardware ecosystems.

## System Architecture

The network architecture is hierarchical and split into three distinct roles:

### 1. Cluster Node (`cluster_node_multi_image_fota/`)
The Cluster Nodes represent edge devices or sensors deployed in the field. They utilize ultra-low power ESB to connect back to the Cluster Head rather than using Wi-Fi directly, drastically improving battery life and increasing scalability.
- **ESB Communication**: Aggregates and sends sensor data reliably to the Cluster Head
- A maximum of 7 cluster nodes can link to a cluster head. Thus, 100 field nodes would need approximately 12-13 cluster heads.

👉 **[Click here to read the Cluster Node Guide (README.md)](cluster_node_multi_image_fota/README.md) (includes build and flash instructions)**

### 2. Cluster Head (`cluster_head_multi_image_fota/`)
The Cluster Head acts as a bridge between the cluster nodes and the central server. It maintains a persistent or semi-persistent Wi-Fi connection to the cloud/WAN and coordinates local devices using Nordic's proprietary ESB protocol. Data packets collected from the cluster nodes are efficiently bundled and backhauled to the central server over the Wi-Fi uplink via MQTT.
- **Wi-Fi Uplink**: Connects to the primary access point.
- **ESB Coordination**: Manages a local cluster of nodes.
- **Dual-Core IPC**: Utilizes Inter-Processor Communication (IPC) to separate the application logic (Application Core) from the networking stack (Network Core).

👉 **[Click here to read the Cluster Head Guide (README.md)](cluster_head_multi_image_fota/README.md) (includes build and flash instructions)**

### 3. Central Server
The Central Server is basically a system with high compute power that serves as the MQTT broker and is capable of advanced processing of data received from the field devices a.k.a cluster nodes.


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

👉 **[Detailed documentation regarding packet and messaging format used in this architecture](data/README.md)**

<!--
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

-->
