#ifndef __MANAGER_THREAD_H__
#define __MANAGER_THREAD_H__

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include "nrfx_timer.h"
#include <zephyr/sys/util.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/logging/log.h>

#include "power_events.h"

#define THREAD_MONITOR_IDX_MAX 10 // max number of application threads to monitor

/**
 * @brief start the manager thread, which will start all other heliogen threads
 *
 * @return k_tid_t
 */
k_tid_t manager_thread_start(void);
void im_alive_cb(int idx, uint32_t ts, uint32_t timeout_ms);
int thread_timeout_check(void);

#endif // __MANAGER_THREAD_H__