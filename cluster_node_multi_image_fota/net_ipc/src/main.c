/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 /*
 * CLUSTER NODE NET CORE FIRMWARE
 * Date: 03-08-2026
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <nrf.h>
#include <esb.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/random/rand32.h>

#include <zephyr/ipc/ipc_service.h>

// LOG_MODULE_REGISTER(esb_netcore, LOG_LEVEL_INF);
LOG_MODULE_REGISTER(esb_netcore, LOG_LEVEL_NONE);

/* Device Identifier */
#define DEVICE_ID 0x14

/* IPC Message Types */
#define ESB_IPC_MSG_TYPE_TX      0x01
#define ESB_IPC_MSG_TYPE_RX      0x02
#define ESB_IPC_MSG_TYPE_STATUS  0x03
#define ESB_IPC_MSG_TYPE_NODE_ID 0x03 /* Match APP core type */
#define ESB_IPC_MSG_TYPE_BATT    0x04
#define ESB_IPC_MSG_TYPE_REQ_BATT 0x05

#define RX_MSGQ_MAX_ITEMS 4
K_MSGQ_DEFINE(esb_rx_msgq, sizeof(struct esb_payload), RX_MSGQ_MAX_ITEMS, 4);

#define IPC_WORK_QUEUE_STACK_SIZE 2048
#define IPC_WORK_QUEUE_PRIORITY 5

K_THREAD_STACK_DEFINE(ipc_work_queue_stack, IPC_WORK_QUEUE_STACK_SIZE);
static struct k_work_q ipc_work_queue;
static struct k_work esb_rx_work;

struct esb_ipc_msg {
	uint8_t type;
	uint8_t length;
	uint8_t data[32];
} __packed;

/* IPC instance */
static struct ipc_ept ep;
static struct ipc_ept_cfg ep_cfg;

static K_SEM_DEFINE(ipc_bound_sem, 0, 1);

static char remote_node_id[33];
static int32_t battery_mv_avg = 0;
static K_SEM_DEFINE(node_id_sem, 0, 1);
static K_SEM_DEFINE(batt_recv_sem, 0, 1);

static struct k_work battery_request_work;
static void battery_request_work_handler(struct k_work *work);

/* ------------------------------------------------------------------ */
/*  Flags shared between ISR (event_handler) and main thread           */
/* ------------------------------------------------------------------ */
static atomic_t do_switch_to_ptx = ATOMIC_INIT(0);
static atomic_t do_switch_to_prx = ATOMIC_INIT(0);
static atomic_t do_retry_ptx     = ATOMIC_INIT(0);

static uint8_t last_tx_attempts = 0;
/* Running total for the currently active packet across retries */
static uint8_t current_packet_total_attempts = 0;

/* Tracks whether we have already done one backoff-retry for the current
 * PTX transmission. Reset on TX_SUCCESS or after giving up. */
static bool tx_retried;

/* ------------------------------------------------------------------ */
/*  PTX configuration — defaults, updated on ring selection in RX     */
/* ------------------------------------------------------------------ */
static uint16_t ptx_retransmit_delay = 600;  /* us between retries   */
static uint16_t ptx_retransmit_count = 3;    /* max retry attempts   */
static int8_t   ptx_tx_output_power  = 0;    /* dBm                  */

/* Ring selected from the last received payload (1, 2, or 3).
 * Lower nibble of the byte at my_packet_index encodes the ring:
 *   0xX1 -> Ring 1: backoff offset 0-500 us
 *   0xX2 -> Ring 2: backoff offset 500-1000 us
 *   0xX3 -> Ring 3: backoff offset 1000-1500 us */
static uint8_t selected_ring = 3;  /* default: Ring 3 */

/* ------------------------------------------------------------------ */
/*  Node MAC-to-packet-index lookup table                              */
/*                                                                      */

#define NODE_COUNT 35  /* 7 nodes per cluster x 5 clusters (A, B, C, D, E) */
#define PKT_RING_MASK 0x0F  /* lower nibble = ring number */

struct node_entry {
	const char *mac_str; /* last 4 chars of node ID string, e.g. "0e70" */
	uint8_t     pkt_idx; /* index into the 42-byte broadcast payload    */
	uint32_t    channel; /* ESB RF channel for this node's cluster head */
	uint8_t     pipe;    /* unique TX pipe for this node (1-7)          */
};

/* Cluster head -> channel mapping (for reference):
 *   A -> ch  3  |  B -> ch  7  |  C -> ch  9
 *   D -> ch 72  |  E -> ch 77
 * Pipes 1-7: all route through base_addr_1, differentiated by prefix byte.
 * Ring is read live from rx_payload.data[pkt_idx] each broadcast. */
static const struct node_entry node_table[NODE_COUNT] = {
	/* Cluster A -- channel 3, pipes 1-7 */
	{ "0f0e",  4, 3, 1 },
	{ "0e86",  5, 3, 2 },
	{ "0e68",  6, 3, 3 },
	{ "0e6e",  7, 3, 4 },
	{ "0e7c",  8, 3, 5 },
	{ "0e52",  9, 3, 6 },
	{ "0e66", 10, 3, 7 },

	/* Cluster B -- channel 7, pipes 1-7 */
	{ "0e62",  4, 7, 1 },
	{ "0e88",  5, 7, 2 }, 
	{ "0e60",  6, 7, 3 },
	{ "0e56",  7, 7, 4 },
	{ "0f7a",  8, 7, 5 },
	{ "0f8a",  9, 7, 6 },
	{ "0e6a", 10, 7, 7 },

	/* Cluster C -- channel 9, pipes 1-7 */
	{ "0e5e",  4, 9, 1 },
	{ "0e54",  5, 9, 2 },
	{ "0e82",  6, 9, 3 },
	{ "0e5c",  7, 9, 4 },
	{ "0e8a",  8, 9, 5 },
	{ "0e7e",  9, 9, 6 },
	{ "0e6c", 10, 9, 7 },

	/* Cluster D -- channel 72, pipes 1-7 */
	{ "0e64",  4, 72, 1 },
	{ "0e74",  5, 72, 2 },
	{ "0e58",  6, 72, 3 },
	{ "0f26",  7, 72, 4 },
	{ "0f20",  8, 72, 5 },
	{ "0f1a",  9, 72, 6 },
	{ "0f1c", 10, 72, 7 },

	/* Cluster E -- channel 77, pipes 1-7 */
	{ "0f14",  4, 77, 1 },
	{ "0f1e",  5, 77, 2 },
	{ "0f04",  6, 77, 3 },
	{ "0f22",  7, 77, 4 },
	{ "0f10",  8, 77, 5 },
	{ "0f0a",  9, 77, 6 },
	{ "0e76", 10, 77, 7 },
};

/* my_packet_index : rx_payload.data[] index this node reads for its ring.*/
static uint8_t  my_packet_index = 0xFF; /* 0xFF = unresolved */
static uint32_t my_channel      = 3;    /* default ch 3 (Cluster A) */
static uint8_t  my_pipe         = 1;    /* default pipe 1 */

static struct esb_payload rx_payload;

/* TX payload: first byte = 0x93 */
static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(0,
	0x93, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

/* RSSI sample from the received broadcast packet */
static volatile uint8_t last_rssi;

/* ------------------------------------------------------------------ */
/*  IPC functions              										  */
/* ------------------------------------------------------------------ */
static int request_battery_from_app(void);

static void battery_request_work_handler(struct k_work *work)
{
    request_battery_from_app();
}

/* Work handler for ESB RX data to be sent via IPC */
static void esb_rx_work_handler(struct k_work *work)
{
	struct esb_payload rx_data;
	struct esb_ipc_msg ipc_msg;

	while (k_msgq_get(&esb_rx_msgq, &rx_data, K_NO_WAIT) == 0) {
		LOG_INF("ESB RX: Received %d bytes: "
			"0x%02x 0x%02x 0x%02x 0x%02x "
			"0x%02x 0x%02x 0x%02x 0x%02x",
			rx_data.length,
			rx_data.data[0], rx_data.data[1],
			rx_data.data[2], rx_data.data[3],
			rx_data.data[4], rx_data.data[5],
			rx_data.data[6], rx_data.data[7]);

		/* Send to app core via IPC */
		ipc_msg.type = ESB_IPC_MSG_TYPE_RX;
		ipc_msg.length = rx_data.length;
		memcpy(ipc_msg.data, rx_data.data, rx_data.length);

		int ret = ipc_service_send(&ep, &ipc_msg,
					   sizeof(ipc_msg.type) +
					   sizeof(ipc_msg.length) +
					   rx_data.length);
		if (ret < 0) {
			LOG_ERR("Failed to send RX data via IPC: %d", ret);
		}
	}
}

/* IPC Endpoint Bound Callback */
static void ep_bound(void *priv)
{
	LOG_INF("IPC endpoint bound");
	k_sem_give(&ipc_bound_sem);
}

/* IPC Endpoint Receive Callback */
static void ep_recv(const void *data, size_t len, void *priv)
{
	const struct esb_ipc_msg *msg = (const struct esb_ipc_msg *)data;

	LOG_INF("IPC RX FROM APP: type=0x%02x, len=%d", msg->type, msg->length);

	if (msg->type == ESB_IPC_MSG_TYPE_NODE_ID) {
		uint8_t copy_len = msg->length > 32 ? 32 : msg->length;
		memcpy(remote_node_id, msg->data, copy_len);
		remote_node_id[copy_len] = '\0';
		LOG_INF("NET Core received Node ID from APP: %s", remote_node_id);
		k_sem_give(&node_id_sem);
	} else if (msg->type == ESB_IPC_MSG_TYPE_BATT) {
		if (msg->length == sizeof(int32_t)) {
			memcpy(&battery_mv_avg, msg->data, sizeof(int32_t));
			LOG_INF("NET Core received Battery: %d mV", battery_mv_avg);
			k_sem_give(&batt_recv_sem);
		}
	}
}

static int request_battery_from_app(void)
{
	struct esb_ipc_msg msg;
	msg.type = ESB_IPC_MSG_TYPE_REQ_BATT;
	msg.length = 0;

	/* Clear any stale battery semaphore */
	k_sem_reset(&batt_recv_sem);

	int ret = ipc_service_send(&ep, &msg, sizeof(msg.type) + sizeof(msg.length));
	if (ret < 0) {
		LOG_ERR("Failed to send battery request: %d", ret);
		return ret;
	}

	/* Increase timeout to 5ms to account for App Core processing and USB/Serial delays */
	ret = k_sem_take(&batt_recv_sem, K_MSEC(5));
	if (ret < 0) {
		LOG_WRN("Battery request timed out after 5ms");
		return ret;
	}

	return 0;
}


/* IPC Initialization */
static int ipc_init(void)
{
	int ret;
	const struct device *ipc_instance;

	LOG_INF("Initializing IPC...");

	/* Initialize battery request work */
	k_work_init(&battery_request_work, battery_request_work_handler);

	/* Get IPC instance */
	ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc_icmsg0));
	if (!device_is_ready(ipc_instance)) {
		LOG_ERR("IPC instance not ready");
		return -ENODEV;
	}

	/* Configure endpoint */
	ep_cfg.name = "esb_ep";
	ep_cfg.cb.bound = ep_bound;
	ep_cfg.cb.received = ep_recv;
	ep_cfg.priv = NULL;

	/* Register endpoint */
	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to open IPC instance: %d", ret);
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &ep, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to register IPC endpoint: %d", ret);
		return ret;
	}

	/* Initialize user work queue for IPC communication */
	k_work_queue_start(&ipc_work_queue,
			   ipc_work_queue_stack,
			   K_THREAD_STACK_SIZEOF(ipc_work_queue_stack),
			   IPC_WORK_QUEUE_PRIORITY,
			   NULL);
	k_thread_name_set(&ipc_work_queue.thread, "ipc_wq");
	
	/* Initialize work item */
	k_work_init(&esb_rx_work, esb_rx_work_handler);

	/* Wait for endpoint to be bound */
	ret = k_sem_take(&ipc_bound_sem, K_SECONDS(5));
	if (ret < 0) {
		LOG_ERR("IPC endpoint binding timeout");
		return ret;
	}

	return 0;
}


/* ------------------------------------------------------------------ */
/*  ESB event handler  (runs in radio IRQ context)                     */
/* ------------------------------------------------------------------ */
void event_handler(struct esb_evt const *event)
{
	switch (event->evt_id) {

	case ESB_EVENT_RX_RECEIVED:
		if (esb_read_rx_payload(&rx_payload) == 0) {
			last_rssi = (uint8_t)NRF_RADIO->RSSISAMPLE;

			LOG_INF("RX: len=%d RSSI=-%u dBm [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
				rx_payload.length,
				last_rssi,
				rx_payload.data[0], rx_payload.data[1],
				rx_payload.data[2], rx_payload.data[3],
				rx_payload.data[4], rx_payload.data[5],
				rx_payload.data[6], rx_payload.data[7]);

			/* If first byte is 0x92 -> request a PTX switch */
			if (rx_payload.data[0] == 0x92) {
				LOG_INF("Trigger byte 0x92 received -> scheduling PTX switch");
				/* Mirror the received second byte in our response */
				tx_payload.data[1] = rx_payload.data[1]; // response to packet request

				/* Only echo back the ring-config byte if our MAC has been resolved.
                 * If my_packet_index is still 0xFF the Node ID lookup hasn't happened
                 * yet (or the MAC isn't in the table) — reading data[0xFF] would be
                 * an out-of-bounds access and reliably returns 0x00. */
                if (my_packet_index != 0xFF &&
                    my_packet_index < rx_payload.length) {
                    tx_payload.data[2] = rx_payload.data[my_packet_index];
                } else {
                    tx_payload.data[2] = 0x00; /* node ID not yet resolved */
                    LOG_WRN("TX pkt[2]=0x00: my_packet_index unresolved (0x%02x) "
                        "— MAC lookup pending or not in node_table",
                        my_packet_index);
                }

				/* Scaled battery value (mV / 20) from last known value */
				uint8_t battery_scaled = (uint8_t)(battery_mv_avg / 20);
				tx_payload.data[3] = battery_scaled;

				LOG_INF("Assigned battery to payload: %d mV (scaled: %d)", battery_mv_avg, battery_scaled);

				tx_payload.data[4] = last_rssi;          /* RSSI of received broadcast */
				tx_payload.data[5] = last_tx_attempts;   /* actual attempts last TX  */

				/* Request fresh battery voltage for NEXT time (from system workqueue) */
				k_work_submit(&battery_request_work);

				LOG_INF("TX Prepared: len=%d [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
					tx_payload.length,
					tx_payload.data[0], tx_payload.data[1],
					tx_payload.data[2], tx_payload.data[3],
					tx_payload.data[4], tx_payload.data[5],
					tx_payload.data[6], tx_payload.data[7]);

				/* Set flag to signal main loop to perform PTX switch */
				atomic_set(&do_switch_to_ptx, 1);
			}

			/* ---- Ring selection ----------------------------------------
			 * Per-node path: read directly from the designated index.
			 * Lower nibble of the byte = ring number (1/2/3).
			 * e.g. 0x23 -> 2nd node in ring 3 -> selected_ring = 3.
			 * Fallback (MAC not yet resolved): scan all bytes as before.
			 * ---------------------------------------------------------- */
			if (my_packet_index != 0xFF &&
			    my_packet_index < rx_payload.length) {
				uint8_t ring_byte = rx_payload.data[my_packet_index];
				uint8_t ring_val  = ring_byte & PKT_RING_MASK;
				if (ring_val >= 1 && ring_val <= 3) {
					if (ring_val != selected_ring) {
						LOG_INF("Ring updated %d->%d "
							"(pkt[%d]=0x%02x)",
							selected_ring, ring_val,
							my_packet_index, ring_byte);
					} else {
						LOG_DBG("Ring %d unchanged (pkt[%d]=0x%02x)",
							ring_val, my_packet_index, ring_byte);
					}
					selected_ring = ring_val;
				} else {
					LOG_WRN("pkt[%d]=0x%02x: ring nibble %d invalid,"
						" keeping ring %d",
						my_packet_index, ring_byte,
						ring_val, selected_ring);
				}
			} else {
				/* Fallback: scan all bytes for legacy indicators */
				for (int i = 0; i < rx_payload.length; i++) {
					uint8_t b = rx_payload.data[i];
					if (b == 0x11 || b == 0x21 || b == 0x31) {
						selected_ring = 1;
						LOG_INF("Ring 1 indicator 0x%02x at byte[%d]", b, i);
						break;
					} else if (b == 0x12 || b == 0x22 || b == 0x32) {
						selected_ring = 2;
						LOG_INF("Ring 2 indicator 0x%02x at byte[%d]", b, i);
						break;
					} else if (b == 0x13 || b == 0x23 || b == 0x33) {
						selected_ring = 3;
						LOG_INF("Ring 3 indicator 0x%02x at byte[%d]", b, i);
						break;
					}
				}
			}
		} else {
			LOG_ERR("Error reading rx payload");
		}
		break;

	case ESB_EVENT_TX_SUCCESS:
		LOG_INF("TX SUCCESS (attempts: %d) -> scheduling PRX switch", event->tx_attempts);
		current_packet_total_attempts += (uint8_t)event->tx_attempts;
		last_tx_attempts = current_packet_total_attempts;
		tx_retried = false; /* reset for next PTX cycle */
		atomic_set(&do_switch_to_prx, 1);
		break;

	case ESB_EVENT_TX_FAILED:
		if (!tx_retried) {
			/* First failure: log the config we failed with, then apply
			 * backoff and schedule a retry with the new config. */
			LOG_WRN("TX FAILED (attempts: %d) with delay=%u count=%u pwr=%d"
				" -> applying backoff and retrying",
				event->tx_attempts,
				ptx_retransmit_delay, ptx_retransmit_count,
				ptx_tx_output_power);
			current_packet_total_attempts += (uint8_t)event->tx_attempts;
			/* NOTE: sys_rand32_get() uses a semaphore and is ISR-unsafe.
			 * The actual delay update happens in the main thread below. */
			if (selected_ring == 1) {
				ptx_retransmit_count += 5;
			} else if (selected_ring == 2) {
				ptx_retransmit_count += 7;
			} else {
				ptx_retransmit_count += 10;
			}
			ptx_tx_output_power   = 3;
			tx_retried = true;
			atomic_set(&do_retry_ptx, 1);
		} else {
			/* Second consecutive failure even after backoff -> give up,
			 * switch back to PRX. */
			LOG_WRN("TX FAILED again after backoff (attempts: %d) "
				"-> switching to PRX", event->tx_attempts);
			
			current_packet_total_attempts += (uint8_t)event->tx_attempts;
			last_tx_attempts = current_packet_total_attempts;

			/* Clear un-transmitted payload from the hardware buffer */
			esb_flush_tx();
			
			tx_retried = false; /* reset for next PTX cycle */
			atomic_set(&do_switch_to_prx, 1);
		}
		break;
	}
}

/* ------------------------------------------------------------------ */
/*  Clock                                                               */
/* ------------------------------------------------------------------ */
static int clocks_start(void)
{
	int err, res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		LOG_ERR("Unable to get the Clock manager");
		return -ENXIO;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
	if (err < 0) {
		LOG_ERR("Clock request failed: %d", err);
		return err;
	}

	do {
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res) {
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
	} while (err);

	LOG_DBG("HF clock started");
	return 0;
}

/* ------------------------------------------------------------------ */
/*  ESB initializers                                                    */
/* ------------------------------------------------------------------ */
static const uint8_t base_addr_0[4]  = {0xE7, 0xE7, 0xE7, 0xE7};
static const uint8_t base_addr_1[4]  = {0xC2, 0xC2, 0xC2, 0xC2};
static const uint8_t addr_prefix[8]  = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};

static int esb_prx_initialize(void)
{
	int err;
	struct esb_config config = ESB_DEFAULT_CONFIG;

	config.protocol          = ESB_PROTOCOL_ESB_DPL;
	config.bitrate           = ESB_BITRATE_1MBPS;
	config.mode              = ESB_MODE_PRX;
	config.event_handler     = event_handler;
	config.selective_auto_ack = true;

	err = esb_init(&config);                              if (err) return err;
	err = esb_set_rf_channel(my_channel);                 if (err) return err;
	err = esb_set_base_address_0(base_addr_0);            if (err) return err;
	err = esb_set_base_address_1(base_addr_1);            if (err) return err;
	/* Listen on pipe 0 only — nodes only receive broadcasts from the
	 * cluster head (pipe 0: base_addr_0 + prefix[0]). Pipes 1-7 are
	 * used exclusively for node->cluster-head TX replies. */
	err = esb_set_prefixes(addr_prefix, 1);
	return err;
}

static int esb_ptx_initialize(void)
{
	int err;
	struct esb_config config = ESB_DEFAULT_CONFIG;

	config.protocol           = ESB_PROTOCOL_ESB_DPL;
	// config.bitrate            = ESB_BITRATE_2MBPS;
	config.bitrate            = ESB_BITRATE_1MBPS;
	config.mode               = ESB_MODE_PTX;
	config.event_handler      = event_handler;
	config.selective_auto_ack = true;
	config.retransmit_delay   = ptx_retransmit_delay;
	config.retransmit_count   = ptx_retransmit_count;
	config.tx_output_power    = ptx_tx_output_power;

	err = esb_init(&config);                              if (err) return err;
	err = esb_set_rf_channel(my_channel);                 if (err) return err;
	err = esb_set_base_address_0(base_addr_0);            if (err) return err;
	err = esb_set_base_address_1(base_addr_1);            if (err) return err;
	err = esb_set_prefixes(addr_prefix, ARRAY_SIZE(addr_prefix));
	return err;
}

/* ------------------------------------------------------------------ */
/*  Role-switch helpers  (call only from main thread, NOT from ISR)    */
/* ------------------------------------------------------------------ */
static int enter_prx(void)
{
	int err;
	uint32_t t0 = k_cycle_get_32();

	/* Cleanly stop the radio before disabling ESB.
	 * esb_disable() alone does NOT stop the radio hardware — it only
	 * disables PPI and IRQs, leaving the radio peripheral running.
	 * esb_stop_rx() issues TASKS_DISABLE and waits for the radio to halt.
	 */
	(void)esb_stop_rx(); /* ignore -EINVAL if not currently in RX state */
	esb_disable();

	err = esb_prx_initialize();
	if (err) { LOG_ERR("PRX init failed: %d", err); return err; }

	err = esb_start_rx();
	if (err) { LOG_ERR("esb_start_rx failed: %d", err); return err; }

	uint32_t us = k_cyc_to_us_near32(k_cycle_get_32() - t0);
	LOG_INF("[TIMING] PRX active: %u us after switch command", us);
	return 0;
}

static int enter_ptx(void)
{
	int err;
	uint32_t t0 = k_cycle_get_32();

	/* Cleanly stop the radio before disabling ESB.
	 * After a PRX receives a packet, the radio continues cycling
	 * (RX -> ACK-TX -> RX) autonomously via hardware SHORTs.
	 * esb_disable() alone does NOT stop this — the radio peripheral
	 * keeps running without an ISR. When esb_ptx_initialize() then
	 * starts the TX state machine, the radio is in an unknown state
	 * and the TX event never fires.
	 * esb_stop_rx() issues TASKS_DISABLE and waits for the radio to halt.
	 */
	(void)esb_stop_rx(); /* ignore -EINVAL if not currently in RX state */
	esb_disable();

	err = esb_ptx_initialize();
	if (err) { LOG_ERR("PTX init failed: %d", err); return err; }
	
	/* Set this node's unique TX pipe so the cluster head can identify
	 * which node is replying (rx_payload.pipe on the cluster head side). */
	tx_payload.pipe = my_pipe;

	/* esb_write_payload() in PTX mode auto-starts TX — do NOT call esb_start_tx() */
	err = esb_write_payload(&tx_payload);
	if (err) {
		LOG_ERR("Payload write failed: %d", err);
		if (err == -ENOMEM) { esb_flush_tx(); }
		return err;
	}

	uint32_t us = k_cyc_to_us_near32(k_cycle_get_32() - t0);
	LOG_INF("[TIMING] PTX active + packet queued: %u us after switch command", us);
	return 0;
}


/* ------------------------------------------------------------------ */
/*  Main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
	int err;

	LOG_INF("ESB configurable sample - default role: PRX");

	err = clocks_start();
	if (err) { return 0; }

	/* Initialize IPC */
	err = ipc_init();
	if (err) {
		LOG_ERR("IPC initialization failed");
		return err;
	}

	// /* Wait for Node ID from App Core */
	// if (k_sem_take(&node_id_sem, K_SECONDS(2)) < 0) {
	// 	LOG_WRN("Timeout waiting for Node ID, proceeding with default");
	// }

	/* Force a small delay to ensure log buffers are clear and strings are fully synchronized */
	// k_sleep(K_MSEC(50));

	/* Resolve packet index from the last 4 chars of the Node ID string. */
	if (remote_node_id[0] != '\0') {
		LOG_INF("Node ID: %s", remote_node_id);

		/* Extract last 4 non-null characters of the string */
		size_t id_len = strlen(remote_node_id);
		const char *mac_suffix_str =
			(id_len >= 4) ? &remote_node_id[id_len - 4] : remote_node_id;

		/* Look up in node table (case-insensitive: node ID uses lowercase hex) */
		for (int i = 0; i < NODE_COUNT; i++) {
			if (strncasecmp(mac_suffix_str, node_table[i].mac_str, 4) == 0) {
				my_packet_index = node_table[i].pkt_idx;
				my_channel      = node_table[i].channel;
				my_pipe         = node_table[i].pipe;
				LOG_INF("Node '%s' -> pkt_idx=%d ch=%u pipe=%u", node_table[i].mac_str, my_packet_index, my_channel, my_pipe);
				break;
			}
		}

		if (my_packet_index == 0xFF) {
			LOG_WRN("MAC suffix '%s' not in node table — ring fallback",
				mac_suffix_str);
		}
	} else {
		LOG_WRN("No Node ID received — ring fallback scan active");
	}

	/* Start in PRX (default) */
	err = esb_prx_initialize();
	if (err) { LOG_ERR("ESB PRX init failed: %d", err); return 0; }

	err = esb_start_rx();
	if (err) { LOG_ERR("esb_start_rx failed: %d", err); return 0; }

	LOG_INF("PRX active - waiting for trigger packet (data[0] == 0x01)");

	/* ---- Main event loop ---- */
	while (1) {
		/*
		 * Switch to PTX:
		 * Triggered by receiving a packet whose first byte is 0x01.
		 * Flag is set in the RX ISR.
		 */
		if (atomic_cas(&do_switch_to_ptx, 1, 0)) {
			LOG_INF("Switching PRX -> PTX");
			
			/* Reset PTX params to defaults for the new cycle */
			ptx_retransmit_delay = 600;
			ptx_retransmit_count = 3;
			ptx_tx_output_power  = 0;
			current_packet_total_attempts = 0;

			/* First attempt stagger to spread out initial transmissions */
			uint32_t stagger_us = 0;
			if (selected_ring == 1) {
				stagger_us = sys_rand32_get() % 251;       /* 0 - 250 us */
			} else if (selected_ring == 2) {
				stagger_us = 500 + sys_rand32_get() % 501; /* 500 - 1000 us */
			} else {
				stagger_us = 1500 + sys_rand32_get() % 501;/* 1500 - 2000 us */
			}
			
			if (stagger_us > 0) {
				LOG_INF("Staggering initial TX by %u us", stagger_us);
				k_busy_wait(stagger_us);
			}

			err = enter_ptx();
			if (err) {
				/* If PTX setup failed, go straight back to PRX */
				LOG_ERR("PTX setup failed, returning to PRX");
				enter_prx();
			}
			/* do_switch_to_prx will be set by the TX event handler
			 * once the single packet is sent (success or failure) */
		}

		/*
		 * Retry PTX with backoff:
		 * Triggered by TX_FAILED. Params already updated in the ISR.
		 * Re-initialize PTX with the new (backed-off) config and
		 * re-queue the same payload.
		 */
		if (atomic_cas(&do_retry_ptx, 1, 0)) {
			/* Safe to call sys_rand32_get() here — main thread, not ISR.
			 * Apply a random backoff offset based on the ring selected from RX. */
			if (selected_ring == 1) {
				/* Ring 1: 600-800 us */
				
				ptx_retransmit_delay += (uint16_t)(sys_rand32_get() % 201);
			} else if (selected_ring == 2) {
				/* Ring 2: 1000-1500 us --> 0-1ms */ 
				ptx_retransmit_delay += (uint16_t)(500 + sys_rand32_get() % 501);
			} else {
				/* Ring 3 (default): 2000-2500 us --> 20-30 ms */ 
				ptx_retransmit_delay += (uint16_t)(1500 + sys_rand32_get() % 501);
			}
			LOG_INF("Retrying PTX with backoff config "
				"(delay=%u count=%u pwr=%d)",
				ptx_retransmit_delay, ptx_retransmit_count,
				ptx_tx_output_power);
			err = enter_ptx();
			if (err) {
				LOG_ERR("PTX retry failed (%d), returning to PRX", err);
				enter_prx();
			}
		}

		/*
		 * Switch back to PRX:
		 * Triggered by TX_SUCCESS event handler.
		 */
		if (atomic_cas(&do_switch_to_prx, 1, 0)) {
			LOG_INF("Switching PTX -> PRX");
			err = enter_prx();
			if (err) {
				LOG_ERR("PRX setup failed: %d", err);
				return 0;
			}
			LOG_INF("PRX active - waiting for next trigger packet");
		}

		k_msleep(1); /* short poll interval */
	}

	return 0;
}
