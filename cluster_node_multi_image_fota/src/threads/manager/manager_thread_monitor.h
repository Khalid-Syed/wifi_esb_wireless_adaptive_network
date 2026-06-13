#ifndef __MANAGER_THREAD_MONITOR_H__
#define __MANAGER_THREAD_MONITOR_H__

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "nrfx_timer.h"
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

#define THREAD_MONITOR_IDX_MAX 10                // max number of application threads to monitor
#define THREAD_MONITOR_TIMEOUT_TOLERANCE_MS 5000 // max number of ms that a thread can be late before it is considered stalled
/**
 * @brief initialize the thread monitor
 *
 */
void thread_monitor_init(void);
/**
 * @brief add a thread to the thread monitor
 *
 * @param idx
 * @param name
 * @param assert
 */
void thread_monitor_add(int idx, const char *name, void (*assert)(void));

/**
 * @brief callback that subthread should call to inticate that they are still alive and not stalled
 *
 * @param idx - index of thread
 * @param ts - timestamp of when callback was called
 * @param timeout_ms - timeout in ms when the next callback should be expected
 */
void im_alive_cb(int idx, uint32_t ts, uint32_t timeout_ms);
/**
 * @brief loops through all thread monitor timers and checks for timeouts.
 *
 * @return int < 0 if timeout detected
 */
int thread_monitor_timeout_check(void);
/**
 * @brief artificially set a thread monitor timeout
 *
 * @param idx
 */
void thread_monitor_timeout_artificial(int idx);

#endif // __MANAGER_THREAD__MONITOR_H__