#ifndef STATUS_MQTT_H
#define STATUS_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "status_obj.h"
#include "mqtt_common.h"

/* module to encode and decode mqtt STATUS JSON payloads */

/*
    TELEMETRY TOPIC
    dt/[platform]/[mac]/status

*/

#define STATUS_PUB_TOPIC "status"
#define STATUS_MQTT_TOPIC_DT_STR "dt/" MQTT_PLATFORM "/%s/status"
#define STATUS_MQTT_DT_QOS MQTT_QOS_1_AT_LEAST_ONCE

int status_mqtt_topic_get(struct mqtt_topic *topic, char* topic_str, size_t topic_str_len);
int status_mqtt_publish(status_t *status);

#endif // STATUS_MQTT_H
