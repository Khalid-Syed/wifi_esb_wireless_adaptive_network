#ifndef TEMPLATE_PUB_MQTT_H
#define TEMPLATE_PUB_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "template_pub_types.h" // example of a data object - make your own
#include "mqtt_common.h"

/* module to encode and decode mqtt TEMPLATE_PUB JSON payloads */

/*
    TELEMETRY TOPIC
    dt/[platform]/[mac]/template_pub

*/
#define TEMPLATE_PUB_TOPIC "template_pub"
#define TEMPLATE_PUB_MQTT_TOPIC_DT_STR "dt/" MQTT_PLATFORM "/%s/template_pub"
#define TEMPLATE_PUB_MQTT_DT_QOS MQTT_QOS_1_AT_LEAST_ONCE

int template_pub_mqtt_topic_get(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len);
int template_pub_mqtt_publish(template_pub_t *template_pub);

#endif // TEMPLATE_PUB_MQTT_H
