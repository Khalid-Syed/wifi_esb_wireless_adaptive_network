#ifndef IPC_HANDLER_H
#define IPC_HANDLER_H

// #include <zephyr.h>
// #include <sys/printk.h>
// #include <logging/log.h>

// LOG_MODULE_REGISTER(ipc_handler);

// static void recv_cb(const void *data, size_t len, void *priv);

// struct esb_ipc_msg;

// const struct esb_ipc_msg *get_latest_rx_msg(void);

#include <stddef.h>

#define ESB_IPC_MSG_TYPE_TX      0x01
#define ESB_IPC_MSG_TYPE_RX      0x02
#define ESB_IPC_MSG_TYPE_STATUS  0x03

struct esb_ipc_msg {
	uint8_t type;
	uint8_t length;
	uint8_t data[32];
} __packed;

const char *get_latest_rx_string(size_t *out_len);
int ipc_send_and_wait(const void *data, size_t len, k_timeout_t timeout);
int ipc_send_msg(struct esb_ipc_msg *msg);

#endif /* IPC_HANDLER_H */