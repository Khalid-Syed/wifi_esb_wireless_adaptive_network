
#include "led_thread.h"
#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>
#include <stdlib.h>
#include "led_patterns.h"
#include "led_timer.h"

#define ON true
#define OFF false
#define MODULE led_timer
LOG_MODULE_DECLARE(MODULE, CONFIG_LED_THREAD_LOG_LEVEL);

static int led_pattern_parse(led_pattern_t *pattern);
static void led_timer_expired(struct k_timer *dummy);
static void led_timer_stopped(struct k_timer *dummy);

K_TIMER_DEFINE(led_timer, led_timer_expired, led_timer_stopped);

#define LED_TIMER_MS 100
static led_timer_state state = {
    .pattern = NULL,
    .on_off = OFF,
    .pulse = 0,
    .repeat = 0,
};

static bool led_timer_enabled = true;

void led_timer_enable(void)
{
    k_timer_start(&led_timer, K_MSEC(LED_TIMER_MS), K_NO_WAIT);
}
void led_timer_disable(void)
{
    k_timer_stop(&led_timer);
}

int led_timer_start(led_pattern_t *pattern)
{
    LOG_DBG("led_timer_start");
    if (pattern == NULL)
    {
        LOG_ERR("led_timer_start: no pattern");
        return -ENOENT;
    }
    else if (pattern == state.pattern) // same pattern, so just reset the repeat counter
    {
        LOG_DBG("led_timer_start: same pattern - reset repeat counter");
        state.repeat = 0;
    }
    else if ((state.pattern == NULL) || (pattern->priority >= state.pattern->priority)) // higher priority pattern, so start it
    {
        state.on_off = OFF;
        state.pulse = 0;
        state.repeat = 0;
        state.pattern = pattern;
        k_timer_start(&led_timer, K_MSEC(100), K_NO_WAIT);
    }
    else
    {
        LOG_DBG("led_timer_start %s surpressed: priority too low. current priority = %d", pattern->name, state.pattern->priority);
    }
    return 0;
}
int led_timer_stop(led_pattern_t *pattern)
{
    if (pattern == state.pattern)
    {
        k_timer_stop(&led_timer);
    }
    return 0;
}

/* Private functions */
static void led_timer_expired(struct k_timer *dummy)
{
    LOG_DBG("led_timer_expired");
    if (led_timer_enabled)
    {
        led_pattern_parse(state.pattern);
    }
}
static void led_timer_stopped(struct k_timer *dummy)
{
    k_timer_stop(&led_timer);
    state.pattern = NULL;
    LOG_DBG("led_timer_stopped");
}

static int led_pattern_parse(led_pattern_t *pattern)
{
    if (state.pattern == NULL)
    {
        LOG_DBG("led_timer_expired: no pattern");
        k_timer_stop(&led_timer);
        return 0;
    }
    led_pulse_t *p = &state.pattern->pulses[state.pulse];
    switch (state.on_off)
    {
    case OFF:
        if (p->on_ms > 0)
        {
            LOG_DBG("on for %d ms", p->on_ms);
            led0_set(true);
            k_timer_start(&led_timer, K_MSEC(p->on_ms), K_NO_WAIT);
            state.on_off = ON;
            break;
        }
        // else fall through
        LOG_DBG("fall through");
    case ON:
        if (p->off_ms > 0)
        {
            LOG_DBG("off for %d ms", p->off_ms);
            led0_set(false);
            k_timer_start(&led_timer, K_MSEC(p->off_ms), K_NO_WAIT);
        }
        state.on_off = OFF;
        state.pulse++;
        break;
    }
    LOG_DBG("pulse %d/%d", state.pulse, pattern->duration);
    if (state.pulse >= pattern->duration)
    {
        if (state.pattern->repeat == LED_PATTERN_REPEAT_INFINITE)
        {
            state.pulse = 0; // repeat pulse pattern forever
        }
        else // increment repeat counter and see if we are done
        {
            state.repeat++;
            state.pulse = 0;
            LOG_DBG("current_repeat %d/%d", state.repeat, pattern->repeat);
            if (state.repeat >= pattern->repeat)
            {
                state.repeat = 0;
                state.pattern = NULL; // finished
                LOG_DBG("led_parse_pattern: finished");
            }
        }
    }
    return 0;
}
// accessor for testing and cli
led_timer_state *led_timer_get_state(void)
{
    return &state;
}