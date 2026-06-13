#include <stdlib.h>
#include "manager_connectivity_monitor.h"
#include "wifi_events.h"
#include "transport_events.h"
#include "cfg.h"

#define MODULE manager
LOG_MODULE_DECLARE(MODULE, CONFIG_MANAGER_THREAD_LOG_LEVEL);

static void manager_connectivity_monitor_timeout_wifi(void);
static void manager_connectivity_monitor_timeout_transport(void);

bool wifi_connected = false;
bool transport_connected = false;
uint64_t wifi_connect_ts = 0;
uint64_t wifi_disconnect_ts = 0;
uint64_t transport_connect_ts = 0;
uint64_t transport_disconnect_ts = 0;

static int wifi_to_ms = MANAGER_CONNECTIVITY_MONITOR_WIFI_TIMEOUT_MS;
static int mqtt_to_ms = MANAGER_CONNECTIVITY_MONITOR_TRANSPORT_TIMEOUT_MS;

int manager_connectivity_monitor_init(void)
{
    manager_connectivity_monitor_wifi_timeout_set(cfg_get()->manager_cfg.wifi_to_ms);
    if (wifi_to_ms == 0)
    {
        LOG_ERR("warning: wifi timeout disabled");
    }
    manager_connectivity_monitor_transport_timeout_set(cfg_get()->manager_cfg.mqtt_to_ms);
    if (mqtt_to_ms == 0)
    {
        LOG_ERR("warning: transport timeout disabled");
    }
    return 0;
}

int manager_connectivity_monitor(void)
{
    int rc = 0;
    if ((wifi_connected) && (transport_connected))
    {
        // this is my happy place
        rc = 0;
    }
    else if ((!wifi_connected) && (wifi_to_ms != 0))
    {
        if (k_uptime_get() - wifi_disconnect_ts > (uint64_t)wifi_to_ms)
        {
            LOG_ERR("wifi disconnected for too long, restarting");
            rc = -1;
            // restart by asserting in this module so assert string will be descriptive
            manager_connectivity_monitor_timeout_wifi();
        }
    }
    else if ((!transport_connected) && (mqtt_to_ms != 0))
    {
        if (k_uptime_get() - transport_disconnect_ts > (uint64_t)mqtt_to_ms)
        {
            LOG_ERR("transport disconnected for too long, restarting");
            rc = -1;
            // restart by asserting in this module so assert string will be descriptive
            manager_connectivity_monitor_timeout_transport();
        }
    }
    return rc;
}

void manager_connectivity_monitor_wifi_timeout_set(int timeout_ms)
{
    wifi_to_ms = timeout_ms;
}
void manager_connectivity_monitor_transport_timeout_set(int timeout_ms)
{
    mqtt_to_ms = timeout_ms;
}

/**
 * @brief listen for wifi events
 *
 * @param aeh
 * @return true
 * @return false
 */
static bool manager_connectivity_wifi_event_handler(const struct app_event_header *aeh)
{
    LOG_DBG("%s", __func__);

    if (is_wifi_status_event(aeh))
    {
        struct wifi_status_event *le = cast_wifi_status_event(aeh);
        // print event
        log_wifi_status_event(aeh);
        LOG_DBG("wifi status event: %s", le->status);
        if (!strcmp(le->status, WIFI_STATUS_EVENT_IP_ACQUIRED))
        {
            wifi_connected = true;
            wifi_connect_ts = k_uptime_get();
        }
        else if (!strcmp(le->status, WIFI_STATUS_EVENT_DISCONNECTED))
        {
            wifi_connected = false;
            wifi_disconnect_ts = k_uptime_get();
        }
        else if (!strcmp(le->status, WIFI_STATUS_EVENT_CONNECTED))
        {
#if CONFIG_NET_DHCPV4
            // do nothing, wait for IP address
#else // not using DHCP, so we must have a static ip and should be good to go
#warning "not using using DHCP"
            wifi_connected = true;
            wifi_connect_ts = k_uptime_get();
#endif
        }
        else
        {
            /*WIFI_STATUS_EVENT_CONNECTING "connecting"*/
            /*WIFI_STATUS_EVENT_CONNECT_FAILED "connect failed"*/
            /*WIFI_STATUS_EVENT_DISCONNECTING "disconnecting"*/
            /*WIFI_STATUS_EVENT_DISCONNECT_FAILED "disconnect failed"*/
        }
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);
    return false;
}

/**
 * @brief listen for transport events
 *
 * @param aeh
 * @return true
 * @return false
 */
static bool manager_connectivity_transport_event_handler(const struct app_event_header *aeh)
{
    LOG_DBG("%s", __func__);

    if (is_transport_status_event(aeh))
    {
        struct transport_status_event *le = cast_transport_status_event(aeh);
        // print event
        log_transport_status_event(aeh);
        LOG_DBG("transport status event: %s", le->status);
        if (!strcmp(le->status, TRANSPORT_STATUS_EVENT_READY))
        {
            transport_connected = true;
            transport_connect_ts = k_uptime_get();
        }
        else if (!strcmp(le->status, TRANSPORT_STATUS_EVENT_DISCONNECTED))
        {
            transport_connected = false;
            transport_disconnect_ts = k_uptime_get();
        }
        else
        { // unhandled events
        }
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);
    return false;
}

static void manager_connectivity_monitor_timeout_wifi(void);
static void manager_connectivity_monitor_timeout_transport(void);

static void manager_connectivity_monitor_timeout_wifi(void)
{
    LOG_ERR("Device will now assert in:");
    LOG_ERR("3... ");
    k_sleep(K_MSEC(1000));
    LOG_ERR("2...");
    k_sleep(K_MSEC(1000));
    LOG_ERR("1...");
    k_sleep(K_MSEC(2000));
    LOG_ERR("Asserting...");
    __ASSERT(false, "wifi timeout");
}
static void manager_connectivity_monitor_timeout_transport(void)
{
    LOG_ERR("Device will now assert in:");
    LOG_ERR("3... ");
    k_sleep(K_MSEC(1000));
    LOG_ERR("2...");
    k_sleep(K_MSEC(1000));
    LOG_ERR("1...");
    k_sleep(K_MSEC(2000));
    LOG_ERR("Asserting...");
    __ASSERT(false, "transport timeout");
}

// subscribe to wi-fi app events
APP_EVENT_LISTENER(manager_wifi_monitor, manager_connectivity_wifi_event_handler);
APP_EVENT_SUBSCRIBE(manager_wifi_monitor, wifi_status_event);

// subscribe to transport events
APP_EVENT_LISTENER(manager_transport_monitor, manager_connectivity_transport_event_handler);
APP_EVENT_SUBSCRIBE(manager_transport_monitor, transport_status_event);
