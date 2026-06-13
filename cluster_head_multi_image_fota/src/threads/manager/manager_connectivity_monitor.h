#ifndef __MANAGER_CONNECTIVITY_MONITOR_H__
#define __MANAGER_CONNECTIVITY_MONITOR_H__

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "nrfx_timer.h"
#include <zephyr/sys/util.h>

int manager_connectivity_monitor_init(void);
int manager_connectivity_monitor(void);
void manager_connectivity_monitor_wifi_timeout_set(int timeout_ms);
void manager_connectivity_monitor_transport_timeout_set(int timeout_ms);

// Sadman's note: reducing the timeout to 5 mins from 3 hours
#define MANAGER_CONNECTIVITY_MONITOR_WIFI_TIMEOUT_MS (5 * 60 * 1000)      // 5 mins
#define MANAGER_CONNECTIVITY_MONITOR_TRANSPORT_TIMEOUT_MS (5 * 60 * 1000) // 5 mins
// #define MANAGER_CONNECTIVITY_MONITOR_WIFI_TIMEOUT_MS (180 * 60 * 1000)      // 3 hours
// #define MANAGER_CONNECTIVITY_MONITOR_TRANSPORT_TIMEOUT_MS (180 * 60 * 1000) // 3 hours

#endif // __MANAGER_CONNECTIVITY_MONITOR_H__
