#ifndef SYS_PUB_MQTT_H
#define SYS_PUB_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "sys_pub_types.h" // example of a data object - make your own
#include "mqtt_common.h"

/* module to encode and decode mqtt SYS_PUB JSON payloads */

/*
    TELEMETRY TOPIC
    dt/[platform]/[mac]/sys

*/
#define SYS_PUB_TOPIC "sys"
#define SYS_PUB_MQTT_TOPIC_DT_STR "dt/" MQTT_PLATFORM "/%s/sys"
#define SYS_PUB_MQTT_DT_QOS MQTT_QOS_1_AT_LEAST_ONCE

#if (CONFIG_MQTT_HELPER_HELIOGEN)
int sys_pub_mqtt_topic_get(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len);
int sys_pub_mqtt_publish(sys_info_t *sys_info);
#else // stubs
#define sys_pub_mqtt_topic_get() \
    do                           \
    {                            \
        return 0;                \
    } while (0)
#define sys_pub_mqtt_publish() \
    do                         \
    {                          \
        return 0;              \
    } while (0)
#endif

#endif // SYS_PUB_MQTT_H
