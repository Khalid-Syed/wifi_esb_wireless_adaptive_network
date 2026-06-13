#ifndef MQTT_COMMON_H
#define MQTT_COMMON_H
#include "mqtt_helper_heliogen.h"

#define MQTT_TOPIC_SIZE_MAX 128
#define MQTT_PLATFORM "proton-node-test"
#define MQTT_RESPONSE_TOPIC_SIZE_MAX 128

// mqtt topics to subscribe and or publish to
#include "reboot_mqtt.h"
#include "status_mqtt.h"
#include "ota_dfu_mqtt.h"
#include "fs_sub_mqtt.h"
#include "http_update.h"
#include "sys_pub_mqtt.h"
#include "remote_pub_sub_mqtt.h"
#include "control_mqtt.h"
#include "start_mqtt.h"
#include "node_id_mqtt.h"
#include "rtt_mqtt.h"

/**
 * @brief get point to list to default topics
 *
 * @param *count
 * @return struct mqtt_topic**
 */
struct mqtt_topic *mqtt_default_topics_get(int *count);
bool mqtt_common_on_mqtt_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);
int mqtt_common_publish(char *topic, char *payload, uint8_t qos);
void mqtt_common_default_topics_subscribe(void);
#endif
