/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 /*
 * CLUSTER HEAD NET CORE FIRMWARE
 * Date: 03-08-2026
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/ipc/ipc_service.h>
#include <nrf.h>
#include <esb.h>

// LOG_MODULE_REGISTER(esb_netcore, LOG_LEVEL_INF); 
LOG_MODULE_REGISTER(esb_netcore, LOG_LEVEL_NONE);

/* IPC Message Types */
#define ESB_IPC_MSG_TYPE_TX      0x01
#define ESB_IPC_MSG_TYPE_RX      0x02
#define ESB_IPC_MSG_TYPE_STATUS  0x03
#define ESB_IPC_MSG_TYPE_NODE_ID 0x03 /* Match APP core type */

/* Message queue to pass RX data from ISR to work queue */
#define RX_MSGQ_MAX_ITEMS 4
K_MSGQ_DEFINE(esb_rx_msgq, sizeof(struct esb_payload), RX_MSGQ_MAX_ITEMS, 4);

/* User work queue for IPC communication (runs in proper thread context) */
#define IPC_WORK_QUEUE_STACK_SIZE 2048
#define IPC_WORK_QUEUE_PRIORITY 5

#define CHANNEL_CH_A_3 3
#define CHANNEL_CH_B_7 7
#define CHANNEL_CH_C_9 9
#define CHANNEL_CH_D_72 72
#define CHANNEL_CH_E_77 77

K_THREAD_STACK_DEFINE(ipc_work_queue_stack, IPC_WORK_QUEUE_STACK_SIZE);
static struct k_work_q ipc_work_queue;
static struct k_work esb_rx_work;

/* ESB IPC Message structure */
struct esb_ipc_msg {
	uint8_t type;
	uint8_t length;
	uint8_t data[32];
} __packed;

/* IPC instance */
static struct ipc_ept ep;
static struct ipc_ept_cfg ep_cfg;

/* Synchronization */
static K_SEM_DEFINE(ipc_bound_sem, 0, 1);

/* Store Received Node ID */
static char remote_node_id[33];
static K_SEM_DEFINE(node_id_sem, 0, 1);

/* ESB state */
static struct esb_payload tx_payload;
static struct esb_payload rx_payload;

/* Forward declarations */
static int esb_radio_init(void);
static int clocks_start(void);

/* Work handler for ESB RX data to be sent via IPC */
static void esb_rx_work_handler(struct k_work *work)
{
	struct esb_payload rx_data;
	struct esb_ipc_msg ipc_msg;

	while (k_msgq_get(&esb_rx_msgq, &rx_data, K_NO_WAIT) == 0) {
		LOG_INF("ESB RX: Received %d bytes on pipe %d: "
			"0x%02x 0x%02x 0x%02x 0x%02x "
			"0x%02x 0x%02x 0x%02x 0x%02x",
			rx_data.length, rx_data.pipe,
			rx_data.data[0], rx_data.data[1],
			rx_data.data[2], rx_data.data[3],
			rx_data.data[4], rx_data.data[5],
			rx_data.data[6], rx_data.data[7]);

		/* Send to app core via IPC */
		ipc_msg.type = ESB_IPC_MSG_TYPE_RX;
		ipc_msg.length = rx_data.length;
		memcpy(ipc_msg.data, rx_data.data, rx_data.length);

		LOG_INF("IPC MSG: type=0x%02x, len=%d", ipc_msg.type, ipc_msg.length);

		int ret = ipc_service_send(&ep, &ipc_msg,
					   sizeof(ipc_msg.type) +
					   sizeof(ipc_msg.length) +
					   rx_data.length);
		if (ret < 0) {
			LOG_ERR("Failed to send RX data via IPC: %d", ret);
		}
	}
}

/* ESB Event Handler */
void esb_evt_handler(struct esb_evt const *event)
{
	switch (event->evt_id) {
	case ESB_EVENT_TX_SUCCESS:
		LOG_INF("ESB TX SUCCESS: Broadcast finished");
		/* Flush RX FIFO before starting RX to remove any stale packets */
		esb_flush_rx();
		/* Switch to PRX mode to listen for responses from field nodes */
		esb_start_rx();
		break;

	case ESB_EVENT_TX_FAILED:
		LOG_ERR("ESB TX FAILED: Retrying/Opening RX anyway...");
		esb_flush_rx();
		/* Open RX window anyway so we don't miss anything */
		esb_start_rx();
		break;

	case ESB_EVENT_RX_RECEIVED:
		LOG_INF("ESB RX: Packet received on radio");
		/* Read received payload */
		if (esb_read_rx_payload(&rx_payload) == 0) {
			/* Queue for processing and submit to user work queue */
			int ret = k_msgq_put(&esb_rx_msgq, &rx_payload, K_NO_WAIT);
			if (ret < 0) {
				LOG_ERR("RX queue full, dropping packet");
			} else {
				/* Submit to user work queue (not system queue) */
				k_work_submit_to_queue(&ipc_work_queue, &esb_rx_work);
			}
		} else {
				LOG_ERR("Error reading ESB RX payload");
		}
		break;

	default:
		LOG_WRN("Unknown ESB event: %d", event->evt_id);
		break;
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

	/* Handle TX command */
	if (msg->type == ESB_IPC_MSG_TYPE_TX) {
		/* Stop RX if currently in the middle of a previous window */
		esb_stop_rx();
		/* Important: Clear any pending RX data in the queue */
		struct esb_payload junk;
		while (k_msgq_get(&esb_rx_msgq, &junk, K_NO_WAIT) == 0);

		/* Prepare ESB payload for Broadcast transmission */
		uint8_t first_byte = msg->data[0];
		/* Logic check: if first byte is 0x91, change it to 0x92 */
		if (first_byte == 0x91) {
			first_byte = 0x92;
		}

		tx_payload = (struct esb_payload)ESB_CREATE_PAYLOAD(0, first_byte);
		tx_payload.length = msg->length;
		
		/* Copy remaining data from the IPC message */
		if (msg->length > 1) {
			memcpy(&tx_payload.data[1], &msg->data[1], msg->length - 1);
		}
		tx_payload.noack = true;

		/* Flush any old TX data and write new payload */
		esb_flush_tx();
		int ret = esb_write_payload(&tx_payload);
		if (ret) {
			LOG_ERR("ESB write_payload failed: %d", ret);
		} else {
			LOG_INF("ESB: write_payload OK, waiting for TX_SUCCESS event...");
		}
	// } else if (msg->type == ESB_IPC_MSG_TYPE_STATUS) {
	// 	/* App Core signals to stop listening (e.g. 800ms window closed) */
	// 	LOG_INF("IPC: Stop RX command received");
	// 	esb_stop_rx();
	} 
	else if (msg->type == ESB_IPC_MSG_TYPE_NODE_ID) {
		uint8_t copy_len = msg->length > 32 ? 32 : msg->length;
		memcpy(remote_node_id, msg->data, copy_len);
		remote_node_id[copy_len] = '\0';
		LOG_INF("NET Core received Node ID from APP: %s", remote_node_id);
		k_sem_give(&node_id_sem);
	}
}

/* IPC Initialization */
static int ipc_init(void)
{
	int ret;
	const struct device *ipc_instance;

	LOG_INF("Initializing IPC...");

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

	LOG_INF("IPC initialized successfully, waiting for Node ID...");
	
	/* Wait for Node ID from App Core */
	ret = k_sem_take(&node_id_sem, K_SECONDS(2));
	if (ret < 0) {
		LOG_WRN("Timeout waiting for Node ID, proceeding with default");
	}

	return 0;
}

/* Start HF Clock */
static int clocks_start(void)
{
	int err;
	int res;
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

/* Initialize ESB Radio */
static int esb_radio_init(void)
{
	int err;

	/* ESB addresses - must match PTX/PRX counterpart */
	uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
	uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
	uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};
	// uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00};

	struct esb_config config = ESB_DEFAULT_CONFIG;

	/* Configure ESB as PTX (Primary Transmitter) */
	config.protocol = ESB_PROTOCOL_ESB_DPL;
	// config.bitrate = ESB_BITRATE_2MBPS;
	config.bitrate = ESB_BITRATE_1MBPS;
	config.mode = ESB_MODE_PTX;  /* Default PTX mode */
	config.event_handler = esb_evt_handler;
	config.selective_auto_ack = true;
	config.retransmit_delay = 250; 
	config.retransmit_count = 0;   /* No retransmissions */

	err = esb_init(&config);
	if (err) {
		LOG_ERR("ESB init failed: %d", err);
		return err;
	}

	err = esb_set_rf_channel(CHANNEL_CH_B_7);
	if (err) {
		LOG_ERR("ESB set RF channel failed: %d", err);
		return err;
	}

	err = esb_set_base_address_0(base_addr_0);
	if (err) {
		LOG_ERR("ESB set base address 0 failed: %d", err);
		return err;
	}

	err = esb_set_base_address_1(base_addr_1);
	if (err) {
		LOG_ERR("ESB set base address 1 failed: %d", err);
		return err;
	}

	err = esb_set_prefixes(addr_prefix, ARRAY_SIZE(addr_prefix));
	if (err) {
		LOG_ERR("ESB set prefixes failed: %d", err);
		return err;
	}

	LOG_INF("ESB initialized successfully (PTX mode, 2.4GHz, 2Mbps)");
	return 0;
}

int main(void)
{
	int err;

	LOG_INF("===========================================");
	LOG_INF("ESB Radio Modem - Network Core");
	LOG_INF("2.4GHz ESB on nRF5340 Internal Radio");
	// LOG_INF("BEFORE FOTA");
	LOG_INF("===========================================");

	/* Start HF clock */
	err = clocks_start();
	if (err) {
		LOG_ERR("Failed to start clocks");
		return err;
	}

	/* Initialize IPC */
	err = ipc_init();
	if (err) {
		LOG_ERR("IPC initialization failed");
		return err;
	}

		/* Verify Node ID before proceeding */
	if (remote_node_id[0] != '\0') {
		LOG_INF("Verified Node ID: %s. Proceeding to ESB init.", remote_node_id);
	} else {
		LOG_WRN("Proceeding to ESB init without Node ID (using default/empty)");
	}

	/* Initialize ESB */
	err = esb_radio_init();
	if (err) {
		LOG_ERR("ESB initialization failed");
		return err;
	}

	/* Device starts idle in PTX mode - no call to esb_start_rx() here */

	LOG_INF("ESB Radio Modem ready - PTX Mode (Default Idle)");
	LOG_INF("Waiting for IPC commands from App Core...");

	/* Return to idle thread - ESB events handled via callbacks */
	return 0;
}
