#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ipc_handler, LOG_LEVEL_INF);

static struct ipc_ept ept;

/* IPC Message Types (match NET core) */
#define ESB_IPC_MSG_TYPE_TX      0x01
#define ESB_IPC_MSG_TYPE_RX      0x02
 
struct esb_ipc_msg {
    uint8_t type;
    uint8_t length;
    uint8_t data[32];
} __packed;

static void recv_cb(const void *data, size_t len, void *priv)
{
	ARG_UNUSED(priv);

	// /* Print as a string (we’ll send without '\0' from NET) */
	// LOG_INF("NET->APP (%u bytes): %.*s",
	// 	(unsigned int)len, (int)len, (const char *)data);

	const struct esb_ipc_msg *msg = (const struct esb_ipc_msg *)data;
    if (msg->type == ESB_IPC_MSG_TYPE_RX) {
        LOG_INF("Wireless RX: %d bytes: %.*s", msg->length, msg->length, msg->data);
        // Process received wireless data here
    }
}