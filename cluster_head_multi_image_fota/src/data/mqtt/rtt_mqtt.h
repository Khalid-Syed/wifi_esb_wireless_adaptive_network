#ifndef RTT_MQTT_H
#define RTT_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "rtt_types.h"
/* module to encode and decode mqtt rtt JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/rtt

    CMD TOPIC RESPONSE
    cmd/[platform]/rtt_res
*/
#define RTT_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/rtt"
#define RTT_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s"
#define RTT_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define RTT_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/rtt_res"
#define RTT_MQTT_CMD_RSP_QOS MQTT_QOS_1_AT_LEAST_ONCE

int rtt_mqtt_topic_get(struct mqtt_topic *topic);
bool rtt_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

#endif // RTT_MQTT_H