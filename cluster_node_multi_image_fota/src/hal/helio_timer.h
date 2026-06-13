#ifndef __HELIO_TIMER_H__
#define __HELIO_TIMER_H__

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <nrfx_timer.h>

#include "motor_constants.h"

void helio_init_stepper_timer(PwmSelect_t block, uint32_t interval_usec);
void helio_stop_stepper_timer(PwmSelect_t block);
void helio_start_stepper_timer(PwmSelect_t block, uint32_t interval_usec);

#endif // __HELIO_TIMER_H__