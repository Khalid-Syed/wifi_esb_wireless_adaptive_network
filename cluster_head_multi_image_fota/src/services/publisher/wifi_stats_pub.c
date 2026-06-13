#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/logging/log.h>
#include "wifi_stats_pub.h"
#include "wifi_stats_pub_mqtt.h"
#include "pubs.h"

#include <zephyr/net/wifi_mgmt.h>

#include "wifi_thread.h"

// Umar: header files and variables to get system time //
#include "time_sync.h"

// Sadman'es edit
#include "id.h"


#define MODULE pubs
LOG_MODULE_DECLARE(MODULE, CONFIG_PUBS_LOG_LEVEL);

// work item for this pub module - use the pubs scheduler for the callback
K_WORK_DELAYABLE_DEFINE(wifi_stats_pub_dwork, pubs_work_handler);

static wifi_stats_pub_t wifi_stats = {0};

char string_time[64];

int wifi_stats_pub_init(void)
{
    int rc = 0;
    return rc;
}
int wifi_stats_pub_get(wifi_stats_pub_t *wifi_stats)
{
#if defined(CONFIG_NET_STATISTICS_WIFI) && \
    defined(CONFIG_NET_STATISTICS_USER_API)
    struct net_if *iface = net_if_get_default();
    struct wifi_iface_status iface_status = {0};

    char id[ID_LEN];

    if (!(id_get(id, sizeof(id))))
    {
        wifi_stats->id_str = id;
	}
	else
	{
        wifi_stats->id_str = "NA";		
	}

    if (fetch_server_time_success) 
    {
        wifi_stats->time_str = get_current_time_string();
    }
    else
    {
        wifi_stats->time_str = "NA";
    }

    int ret2;

    ret2 = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, 
                    &iface_status, sizeof(iface_status));

    if (!ret2)
    {
        wifi_stats->rssi = iface_status.rssi;
    }

    return 0;
#else
    LOG_INF("WiFi statistics not supported.");
    return -1;
#endif /* CONFIG_NET_STATISTICS_WIFI && CONFIG_NET_STATISTICS_USER_API */
}

int wifi_stats_pub(void)
{
    LOG_DBG("");
    if (wifi_stats_pub_get(&wifi_stats) == 0)
    {
        wifi_stats_pub_mqtt_publish(&wifi_stats);
    }
    return 0;
}
