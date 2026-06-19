# Hybrid Network Packet & Message Structure

This document explains the packet and messaging format used for communication between the Cloud (MQTT), the Cluster Head Gateway, and the Cluster Nodes (ESB).

---

## 1. Packet Identifiers (First Byte)

The first byte of every message/packet defines its purpose, routing path, and protocol layer:

| Byte Value | Message Type | Direction | Protocol | Description |
|---|---|---|---|---|
| **`0x91`** | **Cloud Trigger Command** | Cloud $\rightarrow$ Cluster Head | MQTT | Injected by the cloud/MQTT broker to trigger an end-to-end Round-Trip Time (RTT) test. |
| **`0x92`** | **Gateway Broadcast Command** | Cluster Head $\rightarrow$ Cluster Nodes | ESB | Broadcasted by the Cluster Head to wake up nodes and synchronize their timing slots. |
| **`0x93`** | **Node Response Packet** | Cluster Nodes $\rightarrow$ Cluster Head | ESB | Transmitted by individual nodes containing sensor telemetry, battery state, and RF performance data. |

---

## 2. Packet Formats

### A. The Broadcast Command (`0x92`)
When the Cluster Head Network Core receives a `0x91` command via IPC from the Application Core, it automatically translates the first byte to `0x92` and broadcasts it over ESB. 

* **Length**: Typically 11 to 32 bytes (variable depending on node count).
* **Byte Layout**:
  * **Byte 0**: `0x92` (Broadcast Packet Type)
  * **Byte 1**: **Sequence ID** (Incremental packet counter, e.g., `0x01` to `0xFF`).
  * **Byte 2–3**: Reserved / Padding.
  * **Byte 4–10**: **Ring configuration mapping** for nodes. Each node reads its assigned index to determine its backoff window ring (Ring 1, 2, or 3) to prevent collision:
    * Lower nibble (`0xX1`, `0xX2`, `0xX3`) maps to **Ring 1**, **Ring 2**, or **Ring 3**.

---

### B. The Node Response Packet (`0x93`)
When a Cluster Node receives the `0x92` broadcast, it processes the trigger, performs a randomized channel-stagger wait (based on its assigned Ring), and sends back an 8-byte response packet.

* **Length**: 8 bytes.
* **Byte Layout**:

| Byte Index | Field | Data Type | Description |
|:---:|---|:---:|---|
| **0** | Packet Type | `uint8_t` | Constant `0x93`. |
| **1** | Sequence ID | `uint8_t` | Mirrored **Sequence ID** from the `0x92` broadcast. |
| **2** | Ring Config | `uint8_t` | Mirrored Ring value assigned to this node (e.g., `0x31` = Ring 1 at index 3). |
| **3** | Battery Telemetry | `uint8_t` | Scaled battery voltage. **Formula**: `Value * 20 = mV` (e.g., `0x9B` $\rightarrow$ 155 $\times$ 20 = 3100 mV). |
| **4** | RSSI | `uint8_t` | Absolute RSSI of the received broadcast packet (dBm). (e.g., `0x19` $\rightarrow$ -25 dBm). |
| **5** | TX Attempts | `uint8_t` | Number of retransmission attempts the node took to send its previous response. |
| **6** | Padding | `uint8_t` | `0x00` |
| **7** | Padding | `uint8_t` | `0x00` |

---

## 3. Real-World Trace Example

Below is an entry from the field test logs (`rtt_results.csv`):
```csv
145,901564800FA8,02:02.9,02:03.0,97.99,6.939483621,YES,0x93 0x91 0x31 0x9B 0x19 0x05 0x00 0x00
```

### Decoding the Hex Payload:
`0x93 0x91 0x31 0x9B 0x19 0x05 0x00 0x00`

1. **`0x93`** (Byte 0): Node Response Packet.
2. **`0x91`** (Byte 1): Sequence ID is `145` (matching the decimal packet ID `145` at the start of the log line).
3. **`0x31`** (Byte 2): Node was assigned to **Ring 1** (lower nibble is `1`).
4. **`0x9B`** (Byte 3): Battery telemetry byte is `155` decimal. `155 * 20 = 3100 mV` (3.1V).
5. **`0x19`** (Byte 4): Received broadcast signal strength was **-25 dBm** (strong signal).
6. **`0x05`** (Byte 5): The node required **5 transmit attempts** to successfully reach the Cluster Head.
7. **`0x00 0x00`** (Bytes 6–7): Padding.
