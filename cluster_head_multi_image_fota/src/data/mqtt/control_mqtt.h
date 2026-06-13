#ifndef CONTROL_MQTT_H
#define CONTROL_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "control_types.h"
/* module to encode and decode mqtt control JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/[mac]/control

    CMD TOPIC RESPONSE
    cmd/[platform]/[mac]/control_res
*/
#define CONTROL_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/%s/control"
#define CONTROL_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s/%s"
#define CONTROL_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define CONTROL_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/%s/control_res"
#define CONTROL_MQTT_CMD_RSP_QOS MQTT_QOS_1_AT_LEAST_ONCE

int control_mqtt_topic_get(struct mqtt_topic *topic);
bool control_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

extern bool start_cla;

#endif // CONTROL_MQTT_H
