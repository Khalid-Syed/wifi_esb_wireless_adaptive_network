
#include <stddef.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "manager_thread.h"

#include "led_thread.h"
#include "power_thread.h"
#include "wifi_thread.h"
#include "mcumgr_service.h"
#include "transport.h"
#include "status_obj.h"
#include "sys_watchdog.h"
#include "manager_thread_monitor.h"
#include "manager_connectivity_monitor.h"
#include "pubs.h"

#ifndef MANAGER_THREAD_STACK_SIZE
#define MANAGER_THREAD_STACK_SIZE 1024
#endif
#ifndef MANAGER_THREAD_PRIORITY
#define MANAGER_THREAD_PRIORITY 5
#endif
#define MODULE manager

LOG_MODULE_REGISTER(MODULE, CONFIG_MANAGER_THREAD_LOG_LEVEL);

K_THREAD_STACK_DEFINE(manager_thread_stack_area, MANAGER_THREAD_STACK_SIZE);
struct k_thread manager_thread_data;

static void manager_thread_entry_point(void *, void *, void *);
extern void sys_watchdog_feed(void);

static int thread_idx = 0;

static volatile bool currently_homing = false;

/**
 * @brief This is where the thread starts
 *
 * @return k_tid_t
 */
k_tid_t manager_thread_start(void)
{
    k_tid_t my_tid = k_thread_create(&manager_thread_data, manager_thread_stack_area,
                                     K_THREAD_STACK_SIZEOF(manager_thread_stack_area),
                                     manager_thread_entry_point,
                                     NULL, NULL, NULL,
                                     MANAGER_THREAD_PRIORITY, 0, K_NO_WAIT);
    return my_tid;
}
/**
 * @brief start up managed sub threads
 *
 * @return int
 */
static int start_managed_threads(void)
{
// start the heliogen application threads
#if CONFIG_LED_THREAD
    LOG_DBG("LED thread start - thread_idx %d", thread_idx);
    thread_monitor_add(thread_idx, "LED", led_thread_stall_assert);
    led_thread_start(im_alive_cb, thread_idx);
    thread_idx++;
#endif
#if CONFIG_POWER_THREAD
    LOG_DBG("POWER thread start - thread_idx %d", thread_idx);
    thread_monitor_add(thread_idx, "POWER", power_thread_stall_assert);
    power_thread_start(im_alive_cb, thread_idx);
    thread_idx++;
#endif
#if CONFIG_MCUMGR
    LOG_DBG("mcumgr thread start - thread_idx  %d", thread_idx);
    thread_monitor_add(thread_idx, "MCU Manager", mcumgr_service_stall_assert);
    mcumgr_service_init(im_alive_cb, thread_idx);
    thread_idx++;
#endif
    // done starting threads
    return 0;
}

/**
 * @brief manager thread entry point
 *
 */
static void manager_thread_entry_point(void *, void *, void *)
{
    LOG_INF("manager Initializing watchdog timer");
    sys_watchdog_init();

    manager_connectivity_monitor_init();
    thread_monitor_init();

    start_managed_threads();

    // loop and feed the watchdog while checking up on thread stalling
    while (1)
    {
        k_sleep(K_MSEC(WDT_FEED_INTERVAL));
        sys_watchdog_feed();
        thread_monitor_timeout_check();
        manager_connectivity_monitor();
    }
}

static void start_wifi_thread()
{
    #if CONFIG_WIFI
    LOG_DBG("wifi thread start - thread_idx  %d", thread_idx);
    thread_monitor_add(thread_idx, "Wi-Fi", wifi_thread_stall_assert);
    wifi_thread_start(im_alive_cb, thread_idx);
    thread_idx++;
    #endif
    #if CONFIG_MQTT_HELPER_HELIOGEN
    LOG_DBG("MQTT thread start - thread_idx  %d", thread_idx);
    thread_monitor_add(thread_idx, "Transport", transport_stall_assert);
    transport_start(im_alive_cb, thread_idx);
    thread_idx++;
    #endif
}

/**
 * @brief manager event handler
 *
 * @param aeh
 * @return true
 * @return false
 */
static bool manager_event_handler(const struct app_event_header *aeh)
{
    if (is_transport_status_event(aeh))
    {
        struct transport_status_event *es = cast_transport_status_event(aeh);
        LOG_DBG("Got transport status: %s", es->status);

        if (!strcmp(es->status, TRANSPORT_STATUS_EVENT_DISCONNECTED))
        {
            // DO SOMETHING
        }
        return false;
    }
    else if (is_power_status_event(aeh))
    {
        struct power_status_event *ps = cast_power_status_event(aeh);

        if (ps->power_state == WIFI_THREAD_START)
        {
            start_wifi_thread();
        }
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, manager_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, power_status_event);
