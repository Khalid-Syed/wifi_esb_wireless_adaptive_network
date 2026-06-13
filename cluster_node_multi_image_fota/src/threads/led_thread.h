#ifndef __LED_THREAD_H__
#define __LED_THREAD_H__

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include "nrfx_timer.h"
#include <zephyr/sys/util.h>
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include "led.h"

// the led thread (timer) is responsible for running the led patterns in response to
// events from the wifi thread or other threads.  It makes the decisions about
// which led pattern to run and when to run it.

/**
 * @brief
 *
 * @param cb - callback to indicate thread is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
int led_thread_start(void(*im_alive_cb), int thread_idx);
/**
 * @brief call this when thread is stalled or otherwise not healthy
 *
 */
void led_thread_stall_assert(void);

/**
 * @brief indicate that led timer is alive and well
 *
 * @param ms - ms until timer expires next
 */
void led_timer_alive(int ms);
#endif // __LED_THREAD_H__