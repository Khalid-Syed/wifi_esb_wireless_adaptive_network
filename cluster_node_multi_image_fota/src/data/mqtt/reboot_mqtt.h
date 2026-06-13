#ifndef REBOOT_MQTT_H
#define REBOOT_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "reboot_types.h"
/* module to encode and decode mqtt reboot JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/[mac]/reboot

    CMD TOPIC RESPONSE
    cmd/[platform]/[mac]/reboot_res
*/
#define REBOOT_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/%s/reboot"
#define REBOOT_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s/%s"
#define REBOOT_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define REBOOT_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/%s/reboot_res"
#define REBOOT_MQTT_CMD_RSP_QOS MQTT_QOS_1_AT_LEAST_ONCE

int reboot_mqtt_topic_get(struct mqtt_topic *topic);
bool reboot_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

#endif // REBOOT_MQTT_H
