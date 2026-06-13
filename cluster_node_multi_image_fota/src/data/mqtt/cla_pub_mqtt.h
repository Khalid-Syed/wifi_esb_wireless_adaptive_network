#ifndef CLA_PUB_MQTT_H
#define CLA_PUB_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "cla_pub_types.h" // example of a data object - make your own
#include "mqtt_common.h"

/* module to encode and decode mqtt CLA_PUB JSON payloads */

/*
    TELEMETRY TOPIC
    dt/[platform]/[mac]/cla

*/

#define CLA_PUB_TOPIC "cla"
#define CLA_PUB_MQTT_TOPIC_DT_STR "dt/" MQTT_PLATFORM "/%s/cla"
#define CLA_PUB_MQTT_DT_QOS MQTT_QOS_1_AT_LEAST_ONCE

#if (CONFIG_MQTT_HELPER_HELIOGEN)
int cla_pub_mqtt_topic_get(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len);
int cla_pub_mqtt_publish(cla_pub_t *cla_pub);
#else // stubs
#define cla_pub_mqtt_topic_get() \
    do                                  \
    {                                   \
        return 0;                       \
    } while (0)
#define cla_pub_mqtt_publish() \
    do                                \
    {                                 \
        return 0;                     \
    } while (0)
#endif
#endif // CLA_PUB_MQTT_H
