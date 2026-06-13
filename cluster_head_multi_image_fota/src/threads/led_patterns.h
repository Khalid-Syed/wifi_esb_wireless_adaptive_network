#ifndef __LED_PATTERN_H__
#define __LED_PATTERN_H__

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include "nrfx_timer.h"
#include <zephyr/sys/util.h>
#include "led.h"

#define LED_PATTERN_REPEAT_INFINITE -1
typedef struct
{
    int on_ms;
    int off_ms;
} led_pulse_t;

typedef struct
{
    char *name;
    led_pulse_t *pulses;
    int duration;
    int repeat;
    int priority;
} led_pattern_t;
led_pattern_t **led_patterns_get(void);
int led_patterns_count_get(void);

// make sure these match the order of the patterns in led_patterns.c -- todo: fix this with compile time asserts
#define LED_PATTERN_BOOT 0
#define LED_PATTERN_CONNECTING 1
#define LED_PATTERN_NETWORK_UNHAPPY 2
#define LED_PATTERN_HEARTBEAT 3

#endif // __LED_PATTERN_H__