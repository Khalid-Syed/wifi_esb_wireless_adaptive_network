#ifndef FS_MQTT_H
#define FS_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "fs_sub_types.h"

/* module to encode and decode mqtt fs JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/[mac]/fs

    CMD TOPIC RESPONSE
    cmd/[platform]/[mac]/fs_res
*/
#define FS_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/%s/fs"
#define FS_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s/%s"
#define FS_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define FS_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/%s/fs_res"
#define FS_MQTT_CMD_RSP_QOS MQTT_QOS_1_AT_LEAST_ONCE

int fs_mqtt_topic_get(struct mqtt_topic *topic);
bool fs_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

#endif // FS_MQTT_H
