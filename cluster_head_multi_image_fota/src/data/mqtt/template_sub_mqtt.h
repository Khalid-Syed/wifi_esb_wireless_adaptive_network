#ifndef TEMPLATE_MQTT_H
#define TEMPLATE_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "template_sub_types.h"

/* module to encode and decode mqtt config JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/[mac]/config

    CMD TOPIC RESPONSE
    cmd/[platform]/[mac]/config_resp
*/
#define CONFIG_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/%s/template"
#define CONFIG_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s/%s"
#define CONFIG_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define CONFIG_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/%s/template_res"
#define CONFIG_MQTT_CMD_RSP_QOS MQTT_QOS_1_AT_LEAST_ONCE

int config_mqtt_topic_get(struct mqtt_topic *topic);
bool config_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

#endif // TEMPLATE_MQTT_H
