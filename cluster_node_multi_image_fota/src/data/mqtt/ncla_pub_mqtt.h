#ifndef NCLA_PUB_MQTT_H
#define NCLA_PUB_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "ncla_pub_types.h" // example of a data object - make your own
#include "mqtt_common.h"

/* module to encode and decode mqtt NCLA_PUB JSON payloads */

/*
    TELEMETRY TOPIC
    dt/[platform]/[mac]/ncla

*/

#define NCLA_PUB_TOPIC "ncla"
#define NCLA_PUB_MQTT_TOPIC_DT_STR "dt/" MQTT_PLATFORM "/%s/ncla"
#define NCLA_PUB_MQTT_DT_QOS MQTT_QOS_1_AT_LEAST_ONCE

#if (CONFIG_MQTT_HELPER_HELIOGEN)
int ncla_pub_mqtt_topic_get(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len);
int ncla_pub_mqtt_publish(ncla_pub_t *ncla_pub);
#else // stubs
#define ncla_pub_mqtt_topic_get() \
    do                                  \
    {                                   \
        return 0;                       \
    } while (0)
#define ncla_pub_mqtt_publish() \
    do                                \
    {                                 \
        return 0;                     \
    } while (0)
#endif
#endif // NCLA_PUB_MQTT_H
