#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_common.h"
#include "transport.h"
#include "transport_events.h"

#define MODULE mqtt_common
LOG_MODULE_REGISTER(MODULE, CONFIG_MQTT_COMMON_LOG_LEVEL);

/* example list of topics
struct mqtt_topic test_topics[] = {
    {.topic = {.utf8 = "test1",
               .size = 5},
     .qos = MQTT_QOS_1_AT_LEAST_ONCE},
    {.topic = {.utf8 = "test2",
               .size = 5},
     .qos = MQTT_QOS_1_AT_LEAST_ONCE}};
*/

static size_t subscription_list_build(struct mqtt_topic *list);

/*** subscriptions  *****/
/* array of functions to get individual default topics to subscribe */
int (*subscription_topic_builders[])(struct mqtt_topic *) = {
    reboot_mqtt_topic_get,
    ota_dfu_mqtt_topic_get,
    fs_mqtt_topic_get,
    remote_pub_mqtt_topic_get,
    control_mqtt_topic_get,
    start_mqtt_topic_get,
    node_id_mqtt_topic_get,
    // rtt_mqtt_topic_get,
};
/* array of callback functions to pass incoming published topics */
bool (*subscription_cb[])(struct mqtt_helper_buf, struct mqtt_helper_buf) = {
    reboot_on_mqtt_publish_cb,
    ota_dfu_on_mqtt_publish_cb,
    fs_on_mqtt_publish_cb,
    remote_pub_on_mqtt_publish_cb,
    control_on_mqtt_publish_cb,
    start_on_mqtt_publish_cb,
    node_id_on_mqtt_publish_cb,
    // rtt_on_mqtt_publish_cb,
};
/* array of topics to subscribe */
struct mqtt_topic default_topics[sizeof(subscription_topic_builders) / sizeof(subscription_topic_builders[0])];

/**
 * @brief get point to list to default topics
 *
 * @return struct mqtt_topics*
 */
struct mqtt_topic *mqtt_default_topics_get(int *count)
{
    *count = subscription_list_build(default_topics);
    return default_topics;
}

bool mqtt_common_on_mqtt_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
    LOG_INF("%s Received payload: \"%.*s\" on topic: \"%.*s\"", __func__, payload.size,
            payload.ptr,
            topic.size,
            topic.ptr);
    for (int i = 0; i < sizeof(subscription_cb) / sizeof(subscription_cb[0]); i++)
    {
        if (true == subscription_cb[i](topic, payload))
        {
            return true;
        }
    }
    LOG_ERR("%s No callback found for topic: \"%.*s\"", __func__, topic.size, topic.ptr);
    return false;
}
int mqtt_common_publish(char *topic, char *payload, uint8_t qos)
{
    // this is a wrapper for transport_publish, which will queue the message
    // this would be a good place to pass option to replace duplicates or other logic
    return transport_publish(topic, payload, qos);
}
/**
 * @brief subscribe to default topics
 * this is called after mqtt connection is established
 *
 */

void mqtt_common_default_topics_subscribe(void)
{
    int count;
    struct mqtt_topic *topics = mqtt_default_topics_get(&count);
    LOG_DBG("Subscribing to %d topics", count);
    transport_subscribe(topics, count, k_uptime_get_32());
}

/********************/
/* private functions*/
/*******************/

static size_t subscription_list_build(struct mqtt_topic *list)
{
    int i = 0;
    // runtime build list of topics to subscribe
    // topics have unique strings that can contain identifiers, so they can't be static
    for (i = 0; i < sizeof(subscription_topic_builders) / sizeof(subscription_topic_builders[0]); i++)
    {
        subscription_topic_builders[i](&list[i]);
    }
    return (size_t)i;
}