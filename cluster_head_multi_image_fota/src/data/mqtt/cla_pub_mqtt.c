#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_common.h"
#include "cla_pub_mqtt.h"
#include "transport.h"
#include "id.h"
#include "cla_pub_json.h" // this would be switched out for protobuf or other as needed

#define MODULE cla_pub_mqtt
LOG_MODULE_REGISTER(MODULE, CONFIG_CLA_PUB_MQTT_LOG_LEVEL);

#define MQTT_TOPIC_GET cla_pub_mqtt_topic_get
#define MQTT_PUBLISH cla_pub_mqtt_publish
// dt topic
#define TOPIC CLA_PUB_MQTT_TOPIC_DT_STR
#define TOPIC_DT_LEN ((sizeof(CLA_PUB_MQTT_TOPIC_DT_STR) + ID_LEN) + 1)
#define QOS_DT CLA_PUB_MQTT_DT_QOS

#define DT_OBJ_TYPE cla_pub_t
#define DT_OBJ_P_TYPE DT_OBJ_TYPE *

#define PAYLOAD_SIZE_MAX CLA_PUB_PAYLOAD_STR_SIZE_MAX

/******************************
        Template Code
*******************************/
int MQTT_TOPIC_GET(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len)
{
    char id[ID_LEN];

    if (!(id_get(id, sizeof(id))))
    {
        snprintf(topic_str, TOPIC_DT_LEN, TOPIC, id);
        topic->topic.utf8 = topic_str;
        topic->topic.size = strlen(topic->topic.utf8);
        topic->qos = QOS_DT;
    }
    else
    {
        __ASSERT(0, "Failed to get id");
    }
    return 0;
}

int MQTT_PUBLISH(DT_OBJ_P_TYPE obj)
{
    LOG_DBG("%s", __func__);
    // build topic
    struct mqtt_topic topic;
    char topic_str[TOPIC_DT_LEN];
    MQTT_TOPIC_GET(&topic, &topic_str[0], sizeof(topic_str));

    // build payload
    char payload_str[PAYLOAD_SIZE_MAX];
    int err = PAYLOAD_ENCODE(obj, payload_str, PAYLOAD_SIZE_MAX);
    if (err == 0)
    {
        // publish
        err = mqtt_common_publish(topic_str, payload_str, QOS_DT);
    }
    return err;
}
