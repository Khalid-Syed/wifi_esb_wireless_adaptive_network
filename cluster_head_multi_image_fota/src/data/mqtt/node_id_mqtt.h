#ifndef NODE_ID_MQTT_H
#define NODE_ID_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "node_id_types.h"
/* module to encode and decode mqtt node_id JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/node_id

    CMD TOPIC RESPONSE
    cmd/[platform]/node_id_res
*/
#define NODE_ID_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/node_id"
#define NODE_ID_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s"
#define NODE_ID_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define NODE_ID_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/node_id_res"
#define NODE_ID_MQTT_CMD_RSP_QOS MQTT_QOS_1_AT_LEAST_ONCE

int node_id_mqtt_topic_get(struct mqtt_topic *topic);
bool node_id_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

#endif // NODE_ID_MQTT_H