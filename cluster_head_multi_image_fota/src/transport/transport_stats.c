#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "transport_priv.h"
#include "transport_stats.h"

/* Declare log module */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(transport, CONFIG_MQTT_TRANSPORT_LOG_LEVEL);

static void session_stats_reset(void);

static transport_stats_t stats = {
    .con = 0,
    .dcon = 0,
    .con_s = 0,
    .dcon_s = 0,
    .con_fail = 0,
    .sub_ack = 0,
    .sub_fail = 0,
    .pub_ack = 0,
    .pub_fail = 0,
    .rx = 0,
    .err = 0,
};

int64_t con_ts;  // connection timestamp
int64_t dcon_ts; // disconnect timestamp

int transport_stats_get(transport_stats_t *tstats)
{
    tstats->con = stats.con;
    tstats->dcon = stats.dcon;
    if (con_ts - dcon_ts > 0) // must be currently connected
    {
        tstats->con_s = (k_uptime_delta(&con_ts)) / 1000;
        tstats->dcon_s = (con_ts - dcon_ts) / 1000;
    }
    else // not connected
    {
        tstats->con_s = 0;
        tstats->dcon_s = (k_uptime_delta(&dcon_ts)) / 1000;
    }
    tstats->con_fail = stats.con_fail;
    tstats->sub_ack = stats.sub_ack;
    tstats->sub_fail = stats.sub_fail;
    tstats->pub_ack = stats.pub_ack;
    tstats->pub_fail = stats.pub_fail;
    tstats->rx = stats.rx;
    tstats->err = stats.err;

    return 0;
}
void transport_stats(enum mqtt_events evt)
{
    switch (evt)
    {
    case MQTT_BROKER_NO_EVENT:
        break;
    case MQTT_BROKER_DISCONNECTED:
        stats.dcon++;                        // total number of disconnects
        dcon_ts = k_uptime_get();            // timestamp in ms
        stats.con = k_uptime_delta(&con_ts); // timestamp in ms
        // when disconneted, set reset connection attempt fail counter
        stats.con_fail = 0;
        break;
    case MQTT_BROKER_CONNECT_TO_BROKER_FAIL:
        stats.con_fail++;
        break;
    case MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS:
        stats.con++; // total number of successful connects
        // when connected, reset reset session specific timers
        session_stats_reset();
        con_ts = k_uptime_get(); // connection timestamp in ms
        // calculate time that device was disconnected
        stats.dcon = k_uptime_delta(&dcon_ts); // ms of disconnection time
        break;
    case MQTT_BROKER_SUBSCRIBE_FAIL:
        break;
    case MQTT_BROKER_SUBSCRIBE_ACK:
        stats.sub_ack++;
        break;
    case MQTT_BROKER_PUBLISH_ACK:
        stats.pub_ack++;
        break;
    case MQTT_BROKER_RX_MSG:
        stats.rx++;
        break;
    case MQTT_BROKER_ERROR:
        stats.err++;
        break;
    default:
        LOG_ERR("unhandled mqtt event %d", evt);
        break;
    }
}

static void session_stats_reset(void)
{
    stats.con_s = 0;
    stats.dcon_s = 0;
    stats.pub_ack = 0;
    stats.pub_fail = 0;
    stats.sub_ack = 0;
    stats.sub_fail = 0;
    stats.rx = 0;
    stats.err = 0;
}