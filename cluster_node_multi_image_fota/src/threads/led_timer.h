#ifndef __LED_TIMER_H__
#define __LED_TIMER_H__

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include "nrfx_timer.h"
#include <zephyr/sys/util.h>
#include "led_patterns.h"

typedef struct
{
    led_pattern_t *pattern;
    bool on_off;
    int pulse;
    int repeat;
} led_timer_state;

// enable/disable the led timer, which runs the led patterns
// this is useful if you want to turn off the led patterns for a while and use led for debugging
// or if you want to turn off the led patterns when the device is in a low power state
void led_timer_enable(void);
void led_timer_disable(void);

// start/stop a led pattern
int led_timer_start(led_pattern_t *pattern);
int led_timer_stop(led_pattern_t *pattern);
led_timer_state *led_timer_get_state(void);
#endif // __LED_TIMER_H__