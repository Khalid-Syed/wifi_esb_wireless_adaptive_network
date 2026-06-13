
#include "pubs.h"
#include "template_pub.h"
#include "wifi_stats_pub.h"
#include "sys_pub.h"
#include "ota_dfu_pub.h"
#include "status_pub.h"
#include "mppt_pub.h"
#include "ncla_pub.h"
#include "cla_pub.h"
#include "pub_table.h"

// ADD TO THIS TABLE TO ADD A PUBLISHER

pub_t pub_table[] = {
    /* // template
        {
            .topic = TEMPLATE_PUB_TOPIC,
            .dwork = &template_pub_dwork,
            .init = template_pub_init,
            .pub = template_pub,
            .initial_interval_ms = TEMPLATE_PUB_INITIAL_INTERVAL_MS,
            .intial_interval_jitter_ms = TEMPLATE_PUB_INITIAL_INTERVAL_JITTER_MS,
            .interval_ms = TEMPLATE_PUB_INTERVAL_MS,
            .interval_jitter_ms = TEMPLATE_PUB_INTERVAL_JITTER_MS,
        },
    */
    {
        .topic = NCLA_PUB_TOPIC,
        .dwork = &ncla_pub_dwork,
        .init = ncla_pub_init,
        .pub = ncla_pub,
        .initial_interval_ms = NCLA_PUB_INITIAL_INTERVAL_MS,
        .intial_interval_jitter_ms = NCLA_PUB_INITIAL_INTERVAL_JITTER_MS,
        .interval_ms = NCLA_PUB_INTERVAL_MS,
        .interval_jitter_ms = NCLA_PUB_INTERVAL_JITTER_MS,
    },
    {
        .topic = CLA_PUB_TOPIC,
        .dwork = &cla_pub_dwork,
        .init = cla_pub_init,
        .pub = cla_pub,
        .initial_interval_ms = CLA_PUB_INITIAL_INTERVAL_MS,
        .intial_interval_jitter_ms = CLA_PUB_INITIAL_INTERVAL_JITTER_MS,
        .interval_ms = CLA_PUB_INTERVAL_MS,
        .interval_jitter_ms = CLA_PUB_INTERVAL_JITTER_MS,
    },
    {
        .topic = STATUS_PUB_TOPIC,
        .dwork = &status_pub_dwork,
        .init = status_pub_init,
        .pub = status_pub,
        .initial_interval_ms = STATUS_PUB_INITIAL_INTERVAL_MS,
        .intial_interval_jitter_ms = STATUS_PUB_INITIAL_INTERVAL_JITTER_MS,
        .interval_ms = STATUS_PUB_INTERVAL_MS,
        .interval_jitter_ms = STATUS_PUB_INTERVAL_JITTER_MS,
    },
    {
        .topic = WIFI_STATS_PUB_TOPIC,
        .dwork = &wifi_stats_pub_dwork,
        .init = wifi_stats_pub_init,
        .pub = wifi_stats_pub,
        .initial_interval_ms = WIFI_STATS_PUB_INITIAL_INTERVAL_MS,
        .intial_interval_jitter_ms = WIFI_STATS_PUB_INITIAL_INTERVAL_JITTER_MS,
        .interval_ms = WIFI_STATS_PUB_INTERVAL_MS,
        .interval_jitter_ms = WIFI_STATS_PUB_INTERVAL_JITTER_MS,
    },
    {
        .topic = SYS_PUB_TOPIC,
        .dwork = &sys_pub_dwork,
        .init = NULL, // the sys pubs init needs to be manually called before manager thread.
        .pub = sys_pub,
        .initial_interval_ms = SYS_PUB_INITIAL_INTERVAL_MS,
        .intial_interval_jitter_ms = SYS_PUB_INITIAL_INTERVAL_JITTER_MS,
        .interval_ms = SYS_PUB_INTERVAL_MS,
        .interval_jitter_ms = SYS_PUB_INTERVAL_JITTER_MS,
    },
    {
        .topic = OTA_DFU_PUB_TOPIC,
        .dwork = &ota_dfu_pub_dwork,
        .init = ota_dfu_pub_init,
        .pub = ota_dfu_pub,
        .initial_interval_ms = OTA_DFU_PUB_INITIAL_INTERVAL_MS,
        .intial_interval_jitter_ms = OTA_DFU_PUB_INITIAL_INTERVAL_JITTER_MS,
        .interval_ms = OTA_DFU_PUB_INTERVAL_MS,
        .interval_jitter_ms = OTA_DFU_PUB_INTERVAL_JITTER_MS,
    },
    {
        .topic = MPPT_PUB_TOPIC,
        .dwork = &mppt_pub_dwork,
        .init = mppt_pub_init,
        .pub = mppt_pub,
        .initial_interval_ms = MPPT_PUB_INITIAL_INTERVAL_MS,
        .intial_interval_jitter_ms = MPPT_PUB_INITIAL_INTERVAL_JITTER_MS,
        .interval_ms = MPPT_PUB_INTERVAL_MS,
        .interval_jitter_ms = MPPT_PUB_INTERVAL_JITTER_MS,
    },
};

int pub_table_size(void){
    return ARRAY_SIZE(pub_table);
}