/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

  /*
 * CLUSTER HEAD APP CORE FIRMWARE
 * Date: 03-08-2026
 */

#include <stdio.h>
#include <string.h>
#include <nrfx_clock.h>
#include <nrfx_gpiote.h>
#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/net/hostname.h>
#include "threads/manager/manager_thread.h"
#include "cfg.h"
#include "boot_count.h"
#include "eol_test_result.h"
#include "app_version.h"
#include "reset_cause.h"
#include "sys_watchdog.h"
#include "sys_pub.h"
#include "retained_ram.h"
#include "heliogen_io.h"

/* Added for immediate ESB->MQTT forwarding */
static char current_res_topic[128];
static K_SEM_DEFINE(res_topic_sem, 1, 1);

extern int publish_response_external(char *rsp_topic, char *node_id, char *latest_ipc_msg);
extern bool id_get(char *id, size_t len);
#define ID_LEN 32

void set_current_res_topic(const char *topic)
{
	k_sem_take(&res_topic_sem, K_FOREVER);
	strncpy(current_res_topic, topic, sizeof(current_res_topic) - 1);
	current_res_topic[sizeof(current_res_topic) - 1] = '\0';
	k_sem_give(&res_topic_sem);
}

// **************************** IPC Operations ****************************
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
// **************************** IPC Operations ****************************

#define MODULE main
LOG_MODULE_REGISTER(MODULE, CONFIG_MAIN_LOG_LEVEL);

// **************************** IPC Operations ****************************
static struct ipc_ept ept;
static K_SEM_DEFINE(bound_sem, 0, 1);

/* IPC Message Types (match NET core) */
#define ESB_IPC_MSG_TYPE_TX      0x01
#define ESB_IPC_MSG_TYPE_RX      0x02
#define ESB_IPC_MSG_TYPE_NODE_ID 0x03
 
struct esb_ipc_msg {
    uint8_t type;
    uint8_t length;
    uint8_t data[32];
} __packed;

static void bound_cb(void *priv)
{
	ARG_UNUSED(priv);
	LOG_INF("ICMsg endpoint bound");
	k_sem_give(&bound_sem);
}

static struct k_sem ipc_rx_sem;
static struct esb_ipc_msg latest_rx_msg;

void process_immediate_esb_rx(const uint8_t *data, size_t len)
{
	char node_id_buf[ID_LEN];
	char *node_id = "NA";
	
	if (!(id_get(node_id_buf, sizeof(node_id_buf)))) {
		node_id = node_id_buf;
	}

	/* Format binary response back to hex string for MQTT */
	char hex_resp[128] = {0};
	char *out_ptr = hex_resp;
	for (int i = 0; i < len; i++) {
		out_ptr += sprintf(out_ptr, "0x%02X%s", data[i], (i == len - 1) ? "" : " ");
	}

	k_sem_take(&res_topic_sem, K_FOREVER);
	if (current_res_topic[0] != '\0') {
		publish_response_external(current_res_topic, node_id, hex_resp);
	}
	k_sem_give(&res_topic_sem);
}

static void recv_cb(const void *data, size_t len, void *priv)
{
	ARG_UNUSED(priv);

	const struct esb_ipc_msg *msg = (const struct esb_ipc_msg *)data;
    if (msg->type == ESB_IPC_MSG_TYPE_RX) {
		LOG_INF("Wireless RX: %d bytes (Hex: ", msg->length);
		for (int i = 0; i < msg->length; i++) {
			printk("%02X ", msg->data[i]);
		}
		printk(")\n");

		/* Forward immediately to MQTT */
		process_immediate_esb_rx(msg->data, msg->length);

		memcpy(&latest_rx_msg, msg, sizeof(struct esb_ipc_msg));
		k_sem_give(&ipc_rx_sem);
    }
}

const char *get_latest_rx_string(size_t *out_len)
{
    if (out_len) {
        *out_len = latest_rx_msg.length;
    }
    return (const char *)latest_rx_msg.data;
}

int ipc_send_and_wait(const void *data, size_t len, k_timeout_t timeout)
{
	struct esb_ipc_msg msg;
	msg.type = ESB_IPC_MSG_TYPE_TX;
	
	/* Copy the actual binary bytes into the IPC message */
	msg.length = len > 32 ? 32 : (uint8_t)len;
	memcpy(msg.data, data, msg.length);

	/* Clear any pending signals before sending */
	while (k_sem_take(&ipc_rx_sem, K_NO_WAIT) == 0);
	
	/* Send the message */
	size_t send_len = sizeof(msg.type) + sizeof(msg.length) + msg.length;
	int ret = ipc_service_send(&ept, &msg, send_len);
	if (ret < 0) {
		LOG_ERR("IPC Send failed: %d", ret);
		return ret;
	}

	/* Wait for the signal from recv_cb */
	return k_sem_take(&ipc_rx_sem, timeout);
}

int ipc_send_msg(struct esb_ipc_msg *msg)
{
	return ipc_service_send(&ept, msg, sizeof(msg->type) + sizeof(msg->length) + msg->length);
}

static void err_cb(void *priv, int reason)
{
	ARG_UNUSED(priv);
	LOG_ERR("IPC error: %d", reason);
}

static const struct ipc_ept_cfg ept_cfg = {
	.name = "icmsg_demo",
	.cb = {
		.bound = bound_cb,
		.received = recv_cb,
		.error = err_cb,
	},
};
// **************************** IPC Operations ****************************

int main(void)
{
	k_sem_init(&ipc_rx_sem, 0, 1);
	// Version 2.0.8-12 commit
	printk("*************PROTON NODE*************\n");

#ifdef CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
						   NRF_CLOCK_HFCLK_DIV_1);
#endif

	k_sleep(K_MSEC(2000)); // wait for USB to enumerate and serial client to connect

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock / MHZ(1));

	boot_count_increment();

	if (retained_ram_init())
	{
		retained_ram_to_files_save(); // store any asserts or fatal errors from previous boot to files
	}

	sys_pub_init(); // init sys info

	LOG_INF("boot_count: %d", boot_count_get());
	LOG_INF("app version: %s", app_version_get());
	LOG_INF("reset cause: %s", reset_cause_get());
	LOG_INF("hostname: %s", net_hostname_get());

	// init configuration settings from storage
	cfg_init();

	if (app_event_manager_init())
	{
		LOG_ERR("Application Event Manager not initialized");
		// TODO: [G5FW-104] handle fatal error in more graceful way - what to do if a critical error occurs?
	}

	manager_thread_start();

#ifdef CONFIG_BOOTLOADER_MCUBOOT
	/* This is needed so that MCUBoot won't revert the update, otherwise at the next reboot, MCUBoot will roll the image back */
	// TODO: [G5FW-138] confirm the image when image has run for a while and/or has connected to to mqtt broker, or
	//  has received a message over wi-fi to manually confirm the image
	boot_write_img_confirmed_multi(0);
	boot_write_img_confirmed_multi(1);
#endif

	// bool led_state = false;

	// **************************** IPC Operations ****************************
	const struct device *ipc = DEVICE_DT_GET(DT_NODELABEL(ipc_icmsg0));
	int ret;

	if (!device_is_ready(ipc)) {
		LOG_ERR("IPC instance not ready");
		return 0;
	}

	/* Per IPC service usage, open instance before registering endpoint. :contentReference[oaicite:8]{index=8} */
	ret = ipc_service_open_instance(ipc);
	if (ret < 0) {
		LOG_ERR("ipc_service_open_instance failed: %d", ret);
		return 0;
	}

	ret = ipc_service_register_endpoint(ipc, &ept, &ept_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint failed: %d", ret);
		return 0;
	}

	k_sem_take(&bound_sem, K_FOREVER);
	LOG_INF("APP ready (waiting for NET message...)");
	// **************************** IPC Operations ****************************

	/* Send Hostname (Node ID) to Net Core */
	const char *hostname = net_hostname_get();
	if (hostname) {
		struct esb_ipc_msg id_msg;
		id_msg.type = ESB_IPC_MSG_TYPE_NODE_ID;
		id_msg.length = strlen(hostname);
		if (id_msg.length > 32) id_msg.length = 32;
		memcpy(id_msg.data, hostname, id_msg.length);
		
		ret = ipc_send_msg(&id_msg);
		if (ret < 0) {
			LOG_ERR("Failed to send Hostname to Net Core: %d", ret);
		} else {
			LOG_INF("Sent Hostname to Net Core: %s", hostname);
		}
	}


	/* Poll if the DTR flag was set */
	while (1)
	{
		k_sleep(K_MSEC(200));
		//nrf_gpio_pin_write(LED2, led_state);
		//nrf_gpio_pin_write(LED1, !led_state);
        //led_state = !led_state;
	}

	return 0;
}

#include <hal/nrf_gpio.h>

// Set GPIOs to known state before zephyr starts
// This allows the SPI FLASH to work correctly
// OTA does not appear to work due to SPI-flash
static int early_init()
{
	nrf_gpio_pin_write(PV_I_EN, 0);
	nrf_gpio_pin_write(THERM_EN, 0);
	nrf_gpio_pin_write(LED2, 0);
	nrf_gpio_pin_write(LED1, 0);

	nrf_gpio_cfg_output(PV_I_EN);
	nrf_gpio_cfg_output(THERM_EN);
	nrf_gpio_cfg_output(LED2);
	nrf_gpio_cfg_output(LED1);

	return 0;
}

SYS_INIT(early_init, PRE_KERNEL_1, 0);