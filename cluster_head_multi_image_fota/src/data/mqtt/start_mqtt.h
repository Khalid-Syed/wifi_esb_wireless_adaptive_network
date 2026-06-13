#ifndef START_MQTT_H
#define START_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "start_types.h"
/* module to encode and decode mqtt start JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/start

    CMD TOPIC RESPONSE
    cmd/[platform]/start_res
*/
#define START_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/start"
#define START_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s"
#define START_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define START_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/start_res"
#define START_MQTT_CMD_RSP_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define CLA_NCLA_TRANSITION_SLEEP_INTERVAL_MS (60 * 1000) // 60 seconds or 1 minute

int start_mqtt_topic_get(struct mqtt_topic *topic);
bool start_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

extern bool start_ncla;

#endif // START_MQTT_H
