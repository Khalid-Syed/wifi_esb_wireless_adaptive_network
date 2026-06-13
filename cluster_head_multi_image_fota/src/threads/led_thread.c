
#include "led_thread.h"
#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>
#include "led_patterns.h"
#include "led_timer.h"
#include "../events/wifi_events.h"

#define MODULE led_timer
LOG_MODULE_REGISTER(MODULE, CONFIG_LED_THREAD_LOG_LEVEL);

static void (*im_alive_cb)(int, uint32_t, uint32_t); // callback to manager thread to indicate thread is healthy
static int thread_idx;                               // index to pass back to im_alive callback

// start led timer
int led_thread_start(void(*cb), int idx)
{
    im_alive_cb = cb;
    thread_idx = idx;

    led0_init();
    led_timer_start(led_patterns_get()[LED_PATTERN_BOOT]);
    return 0;
}

void led_thread_stall_assert(void)
{
    LOG_ERR("led thread stalled");
    k_sleep(K_MSEC(1000));
    __ASSERT(false, "led thread stalled");
}

void led_timer_alive(int ms)
{
    if (ms != 0)
    {
        im_alive_cb(thread_idx, k_uptime_get_32(), ms);
    }
    else
    {
        im_alive_cb(thread_idx, 0, 0); // stop the thread monitor on this thread
    }
}

static bool led_thread_event_handler(const struct app_event_header *aeh)
{

    if (is_wifi_status_event(aeh))
    {
        struct wifi_status_event *le = cast_wifi_status_event(aeh);
        // print event
        log_wifi_status_event(aeh);
        LOG_DBG("wifi status event: %s", le->status);
        // TODO: dont use strcmp
        if (!strcmp(le->status, WIFI_STATUS_EVENT_CONNECTING))
        {
            led_timer_start(led_patterns_get()[LED_PATTERN_CONNECTING]);
        }
        else if (!strcmp(le->status, WIFI_STATUS_EVENT_CONNECT_FAILED))
        {
            led_timer_start(led_patterns_get()[LED_PATTERN_NETWORK_UNHAPPY]);
        }
        else if (!strcmp(le->status, WIFI_STATUS_EVENT_CONNECTED))
        {
            // led_timer_start()
        }
        else if (!strcmp(le->status, WIFI_STATUS_EVENT_DISCONNECTING))
        {
            // led_timer_start()
        }
        else if (!strcmp(le->status, WIFI_STATUS_EVENT_DISCONNECT_FAILED))
        {
            // led_timer_start()
        }
        else if (!strcmp(le->status, WIFI_STATUS_EVENT_DISCONNECTED))
        {
            led_timer_start(led_patterns_get()[LED_PATTERN_NETWORK_UNHAPPY]);
        }
        else if (!strcmp(le->status, WIFI_STATUS_EVENT_IP_ACQUIRED))
        {
            led_timer_start(led_patterns_get()[LED_PATTERN_HEARTBEAT]);
        }
        else
        {
            LOG_ERR("unknown wifi status event: %s", le->status);
        }
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);
    return false;
}

APP_EVENT_LISTENER(MODULE, led_thread_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, wifi_status_event);
