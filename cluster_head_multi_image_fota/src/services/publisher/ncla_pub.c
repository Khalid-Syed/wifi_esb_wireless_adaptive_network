#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/logging/log.h>
#include "ncla_pub.h"
#include "ncla_pub_mqtt.h"
#include "pubs.h"
#include <zephyr/net/wifi_mgmt.h>
#include "wifi_thread.h"
#include "time_sync.h"
#include "boot_count.h"
#include "reset_cause.h"
#include "get_random_number.h"

#include "ipc_handler.h"

#define MODULE pubs
LOG_MODULE_DECLARE(MODULE, CONFIG_PUBS_LOG_LEVEL);

// work item for this pub module - use the pubs scheduler for the callback
K_WORK_DELAYABLE_DEFINE(ncla_pub_dwork, pubs_work_handler);

static ncla_pub_t ncla = {0};

char float_buf_az_ncla[32];
char float_buf_el_ncla[32];

int ncla_pub_init(void)
{
    int rc = 0;
    return rc;
}
int ncla_pub_get(ncla_pub_t *ncla)
{
    // get timestamp
    if (fetch_server_time_success) 
    {
        ncla->time_str = get_current_time_string();
    }
    else
    {
        ncla->time_str = "NA";
    }

    // get rssi
    struct net_if *iface = net_if_get_default();
    struct wifi_iface_status iface_status = {0};

    int ret;

    ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, 
                    &iface_status, sizeof(iface_status));

    if (!ret)
    {
        ncla->rssi = iface_status.rssi;
    }
    else
    {
        ncla->rssi = 0; // default value if unable to get RSSI
    }

    // get boot info
    ncla->boot_info.uptime = k_uptime_get() / 1000; // report uptime in seconds
    ncla->boot_info.boot_count = boot_count_get();
    ncla->boot_info.reboot_cause = reset_cause_get();
    
    // get random azimuth and elevation angles
    ncla->az = float_buf_az_ncla;
    ncla->el = float_buf_el_ncla;
    snprintf(ncla->az, 32, "%.5f", get_random_number(AZ_MIN, AZ_MAX));
    snprintf(ncla->el, 32, "%.5f", get_random_number(EL_MIN, EL_MAX));

    // get operation string
    ncla->operation = "NCLA";

    // Get the latest received IPC message
    size_t len;
    const char *str = get_latest_rx_string(&len);
    ncla->latest_ipc_msg = str;

    return 0;
}

int ncla_pub(void)
{
    LOG_DBG("");
    if (ncla_pub_get(&ncla) == 0)
    {
        ncla_pub_mqtt_publish(&ncla);
    }
    return 0;
}
