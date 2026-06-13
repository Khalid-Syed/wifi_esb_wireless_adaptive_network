#ifndef __HELIO_PWM_H__
#define __HELIO_PWM_H__

#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "motor_constants.h"

static const uint16_t OFF_DUTY_CNT = 256;

int32_t helio_pwm_init();
void update_duty_block(uint16_t value);
void turn_off_pwm();
void start_pwm_sequence();

#endif // __HELIO_PWM_H__