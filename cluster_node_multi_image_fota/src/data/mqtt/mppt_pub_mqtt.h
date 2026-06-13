#ifndef MPPT_PUB_MQTT_H
#define MPPT_PUB_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "mppt_pub_types.h" // example of a data object - make your own
#include "mqtt_common.h"

/* module to encode and decode mqtt MPPT_PUB JSON payloads */

/*
    TELEMETRY TOPIC
    dt/[platform]/[mac]/mppt

*/

#define MPPT_PUB_TOPIC "mppt"
#define MPPT_PUB_MQTT_TOPIC_DT_STR "dt/" MQTT_PLATFORM "/%s/mppt"
#define MPPT_PUB_MQTT_DT_QOS MQTT_QOS_1_AT_LEAST_ONCE

int mppt_pub_mqtt_topic_get(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len);
int mppt_pub_mqtt_publish(mppt_pub_t *mppt_pub);

#endif // MPPT_PUB_MQTT_H
