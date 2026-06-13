#ifndef __WIFI_STATS_H__
#define __WIFI_STATS_H__
#include "wifi_stats_pub_types.h"
#include "wifi_stats_pub_mqtt.h"

#define WIFI_STATS_PUB_INITIAL_INTERVAL_MS (15000) // TODO: get from config file
#define WIFI_STATS_PUB_INITIAL_INTERVAL_JITTER_MS (3000)
#define WIFI_STATS_PUB_INTERVAL_MS (10000) // 10 seconds  TODO: get from config file , will want slower at scale
#define WIFI_STATS_PUB_INTERVAL_JITTER_MS 0

// delayable work item - etern for pub scheduler
extern struct k_work_delayable wifi_stats_pub_dwork;

/**
 * @brief init wifi stats module
 *
 * @return int
 */
int wifi_stats_pub_init(void);

/**
 * @brief get wifi_stats struct
 *
 * @return wifi_statsinfo_t*
 */
int wifi_stats_pub_get(wifi_stats_pub_t *wifi_stats);

/**
 * @brief send an mqqt message with wifi statistics
 *
 * @return int
 */
int wifi_stats_pub(void);

#endif