#ifndef REMOTE_PUB_MQTT_H
#define REMOTE_PUB_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "remote_pub_sub_types.h"

/* module to encode and decode mqtt config JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/[mac]/config

    CMD TOPIC RESPONSE
    cmd/[platform]/[mac]/config_resp
*/
#define REMOTE_PUB_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/%s/remote_pub"
#define REMOTE_PUB_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s/%s"
#define REMOTE_PUB_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define REMOTE_PUB_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/%s/remote_pub_res"
#define REMOTE_PUB_MQTT_CMD_RSP_QOS MQTT_QOS_0_AT_MOST_ONCE

int remote_pub_mqtt_topic_get(struct mqtt_topic *topic);
bool remote_pub_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

#endif // REMOTE_PUB_MQTT_H
