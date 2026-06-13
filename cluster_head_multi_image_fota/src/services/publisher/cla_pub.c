#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/logging/log.h>
#include "cla_pub.h"
#include "cla_pub_mqtt.h"
#include "pubs.h"
#include <zephyr/net/wifi_mgmt.h>
#include "wifi_thread.h"
#include "time_sync.h"
#include "boot_count.h"
#include "reset_cause.h"
#include "get_random_number.h"

#define MODULE pubs
LOG_MODULE_DECLARE(MODULE, CONFIG_PUBS_LOG_LEVEL);

// work item for this pub module - use the pubs scheduler for the callback
K_WORK_DELAYABLE_DEFINE(cla_pub_dwork, pubs_work_handler);

static cla_pub_t cla = {0};

char float_buf_az_cla[32];
char float_buf_el_cla[32];

int cla_pub_init(void)
{
    int rc = 0;
    return rc;
}
int cla_pub_get(cla_pub_t *cla)
{
    // get timestamp
    if (fetch_server_time_success) 
    {
        cla->time_str = get_current_time_string();
    }
    else
    {
        cla->time_str = "NA";
    }

    // get rssi
    struct net_if *iface = net_if_get_default();
    struct wifi_iface_status iface_status = {0};

    int ret;

    ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, 
                    &iface_status, sizeof(iface_status));

    if (!ret)
    {
        cla->rssi = iface_status.rssi;
    }
    else
    {
        cla->rssi = 0; // default value if unable to get RSSI
    }

    // get boot info
    cla->boot_info.uptime = k_uptime_get() / 1000; // report uptime in seconds
    cla->boot_info.boot_count = boot_count_get();
    cla->boot_info.reboot_cause = reset_cause_get();
    
    // get random azimuth and elevation angles
    cla->az = float_buf_az_cla;
    cla->el = float_buf_el_cla;
    snprintf(cla->az, 32, "%.5f", get_random_number(AZ_MIN, AZ_MAX));
    snprintf(cla->el, 32, "%.5f", get_random_number(EL_MIN, EL_MAX));

    // get operation string
    cla->operation = "CLA";

    return 0;
}

int cla_pub(void)
{
    LOG_DBG("");
    if (cla_pub_get(&cla) == 0)
    {
        cla_pub_mqtt_publish(&cla);
    }
    return 0;
}
