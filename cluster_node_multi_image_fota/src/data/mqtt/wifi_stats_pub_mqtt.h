#ifndef WIFI_STATS_PUB_MQTT_H
#define WIFI_STATS_PUB_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "wifi_stats_pub_types.h" // example of a data object - make your own
#include "mqtt_common.h"

/* module to encode and decode mqtt WIFI_STATS_PUB JSON payloads */

/*
    TELEMETRY TOPIC
    dt/[platform]/[mac]/wifi_stats_pub

*/

#define WIFI_STATS_PUB_TOPIC "wifi_stats"
#define WIFI_STATS_PUB_MQTT_TOPIC_DT_STR "dt/" MQTT_PLATFORM "/%s/wifi_stats"
#define WIFI_STATS_PUB_MQTT_DT_QOS MQTT_QOS_1_AT_LEAST_ONCE

#if (CONFIG_MQTT_HELPER_HELIOGEN)
int wifi_stats_pub_mqtt_topic_get(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len);
int wifi_stats_pub_mqtt_publish(wifi_stats_pub_t *wifi_stats_pub);
#else // stubs
#define wifi_stats_pub_mqtt_topic_get() \
    do                                  \
    {                                   \
        return 0;                       \
    } while (0)
#define wifi_stats_pub_mqtt_publish() \
    do                                \
    {                                 \
        return 0;                     \
    } while (0)
#endif
#endif // WIFI_STATS_PUB_MQTT_H
