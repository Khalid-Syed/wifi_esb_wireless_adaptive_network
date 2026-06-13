#ifndef __POWER_THREAD_H__
#define __POWER_THREAD_H__

#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <nrfx_saadc.h>
#include <nrfx_qdec.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <zephyr/smf.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#include "cfg.h"
#include "heliogen_io.h"
#include "helio_ntc.h"
#include "helio_pid.h"
#include "helio_pwm.h"
#include "helio_saadc.h"
#include "helio_timer.h"

#include "mqtt_common.h"
#include "power_obj.h"
#include "power_events.h"
#include "storage_counter.h"

enum MpptPowerState
{
    OFF,
    SCAN,
    TRACK
};


void saadc_handler(nrfx_saadc_evt_t const * p_event);
void start_adc(bool calibrate, uint32_t adc_period_ms);

/**
 * @brief
 *
 * @param cb - callback to indicate thread is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
k_tid_t power_thread_start(void(*cb), int idx);
/**
 * @brief call this when thread is stalled or otherwise not healthy
 *
 */
void power_thread_stall_assert(void);

#endif