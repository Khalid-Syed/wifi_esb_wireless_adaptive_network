/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/ipc/ipc_service.h>
#include <nrf.h>
#include <esb.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/random/rand32.h>

LOG_MODULE_REGISTER(esb_netcore, LOG_LEVEL_INF);

/* IPC Message Types */
#define ESB_IPC_MSG_TYPE_TX      0x01
#define ESB_IPC_MSG_TYPE_RX      0x02
#define ESB_IPC_MSG_TYPE_STATUS  0x03

/* Message queue to pass RX data from ISR to work queue */
#define RX_MSGQ_MAX_ITEMS 4
K_MSGQ_DEFINE(esb_rx_msgq, sizeof(struct esb_payload), RX_MSGQ_MAX_ITEMS, 4);

/* User work queue for IPC communication (runs in proper thread context) */
#define IPC_WORK_QUEUE_STACK_SIZE 2048
#define IPC_WORK_QUEUE_PRIORITY 5

/* Device Identifier */
#define DEVICE_ID 0x14

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

/* ------------------------------------------------------------------ */
/*  Flags shared between ISR (event_handler) and main thread           */
/* ------------------------------------------------------------------ */
static atomic_t do_switch_to_ptx = ATOMIC_INIT(0); /* set by RX handler        */
static atomic_t do_switch_to_prx = ATOMIC_INIT(0); /* set by TX_SUCCESS handler */
static atomic_t do_retry_ptx     = ATOMIC_INIT(0); /* set by TX_FAILED handler  */

/* Tracks whether we have already done one backoff-retry for the current
 * PTX transmission. Reset on TX_SUCCESS or after giving up. */
static bool tx_retried;

/* ------------------------------------------------------------------ */
/*  PTX configuration — defaults, updated live if 0x33 found in RX    */
/* ------------------------------------------------------------------ */
static uint16_t ptx_retransmit_delay = 600;  /* us between retries   */
static uint16_t ptx_retransmit_count = 3;    /* max retry attempts   */
static int8_t   ptx_tx_output_power  = 3;    /* dBm                  */

/* Payload buffers */
static struct esb_payload rx_payload;

/* TX payload: first byte = 0x93 */
static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(0,
	0x93, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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


/* ------------------------------------------------------------------ */
/*  ESB event handler  (runs in radio IRQ context)                     */
/* ------------------------------------------------------------------ */
void event_handler(struct esb_evt const *event)
{
	switch (event->evt_id) {

	case ESB_EVENT_RX_RECEIVED:
		if (esb_read_rx_payload(&rx_payload) == 0) {
			LOG_INF("RX: len=%d [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]",
				rx_payload.length,
				rx_payload.data[0], rx_payload.data[1],
				rx_payload.data[2], rx_payload.data[3],
				rx_payload.data[4], rx_payload.data[5],
				rx_payload.data[6], rx_payload.data[7]);

			/* If first byte is 0x92 -> request a PTX switch */
			if (rx_payload.data[0] == 0x92) {
				LOG_INF("Trigger byte 0x92 received -> scheduling PTX switch");
				/* Mirror the received second byte in our response */
				tx_payload.data[1] = rx_payload.data[1]; // response to packet request
				tx_payload.data[2] = DEVICE_ID;          // identify this device in the response
				atomic_set(&do_switch_to_ptx, 1);
			}

			/* Scan every byte for 0x33 -> update PTX config */
			for (int i = 0; i < rx_payload.length; i++) {
				if (rx_payload.data[i] == 0x33) {
					LOG_INF("0x33 found at byte[%d] -> updating PTX config: "
						"retransmit_delay=1000, retransmit_count=5, "
						"tx_output_power=0", i);
					ptx_retransmit_delay = 1000;
					ptx_retransmit_count = 5;
					ptx_tx_output_power  = 0;
					break;
				}
			}
		} else {
			LOG_ERR("Error reading rx payload");
		}
		break;

	case ESB_EVENT_TX_SUCCESS:
		LOG_INF("TX SUCCESS (attempts: %d) -> scheduling PRX switch",
			event->tx_attempts);
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
			// ptx_retransmit_delay += (uint16_t)(sys_rand32_get() % 501); // Ring 1
			// ptx_retransmit_delay += (uint16_t)(500 + sys_rand32_get() % 501); // Ring 2
			// ptx_retransmit_delay += (uint16_t)(1000 + sys_rand32_get() % 501); // Ring 3
			/* NOTE: sys_rand32_get() uses a semaphore and is ISR-unsafe.
			 * The actual delay update happens in the main thread below. */
			ptx_retransmit_count += 5;
			ptx_tx_output_power   = 3;
			tx_retried = true;
			atomic_set(&do_retry_ptx, 1);
		} else {
			/* Second consecutive failure even after backoff -> give up,
			 * switch back to PRX. */
			LOG_WRN("TX FAILED again after backoff (attempts: %d) "
				"-> switching to PRX", event->tx_attempts);
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
static const uint8_t addr_prefix[8]  = {0xE6, 0xC2, 0xC3, 0xC4,
					 0xC5, 0xC6, 0xC7, 0xC8};

static int esb_prx_initialize(void)
{
	int err;
	struct esb_config config = ESB_DEFAULT_CONFIG;

	config.protocol          = ESB_PROTOCOL_ESB_DPL;
	config.bitrate           = ESB_BITRATE_2MBPS;
	config.mode              = ESB_MODE_PRX;
	config.event_handler     = event_handler;
	config.selective_auto_ack = true;

	err = esb_init(&config);                              if (err) return err;
	err = esb_set_base_address_0(base_addr_0);            if (err) return err;
	err = esb_set_base_address_1(base_addr_1);            if (err) return err;
	err = esb_set_prefixes(addr_prefix, ARRAY_SIZE(addr_prefix));
	return err;
}

static int esb_ptx_initialize(void)
{
	int err;
	struct esb_config config = ESB_DEFAULT_CONFIG;

	config.protocol           = ESB_PROTOCOL_ESB_DPL;
	config.bitrate            = ESB_BITRATE_2MBPS;
	config.mode               = ESB_MODE_PTX;
	config.event_handler      = event_handler;
	config.selective_auto_ack = true;
	config.retransmit_delay   = ptx_retransmit_delay;
	config.retransmit_count   = ptx_retransmit_count;
	config.tx_output_power    = ptx_tx_output_power;

	err = esb_init(&config);                              if (err) return err;
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
			/* Safe to call sys_rand32_get() here — main thread, not ISR */
			ptx_retransmit_delay += (uint16_t)(1000 + sys_rand32_get() % 501); // Ring 3
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
