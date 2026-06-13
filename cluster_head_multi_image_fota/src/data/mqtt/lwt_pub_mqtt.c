#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_common.h"
#include "lwt_pub_mqtt.h"
#include "sys_pub.h"

/** LAST WILL AND TESTAMENT */
// We will use the sys topic as our LWT

#include "sys_pub_mqtt.h"
#include "sys_pub_json.h"
#include "transport_events.h"
#include "id.h"
#include "string_utils.h"

LOG_MODULE_DECLARE(transport, CONFIG_MQTT_TRANSPORT_LOG_LEVEL);

#define TOPIC_DT_LEN ((sizeof(SYS_PUB_MQTT_TOPIC_DT_STR) + ID_LEN) + 1)
#define QOS_DT SYS_PUB_MQTT_DT_QOS
#define PAYLOAD_SIZE_MAX SYS_PUB_PAYLOAD_STR_SIZE_MAX

static struct mqtt_topic will_topic;
static char will_topic_str[TOPIC_DT_LEN];
static struct mqtt_utf8 will_message;
static char will_message_str[PAYLOAD_SIZE_MAX];
static sys_info_t sys;

int lwt_create(void)
{
    // create last will and testiment mqtt message
    // this will be sent to the broker when connecting, and if a disconnect is detected by the broker,
    // the LWT topic/message will be published by the broker to all subscribed clients.
    // This will let them know that the node has disconnected unexpectedly.

    // build topic
    sys_pub_mqtt_topic_get(&will_topic, &will_topic_str[0], sizeof(will_topic_str));
    will_topic.qos = QOS_DT;

    // build message
    sys_pub_get(&sys);
    sys.con_status = TRANSPORT_STATUS_EVENT_DISCONNECTED; // LWT message will indicate node id disconnected
    sys.con_s = 0;
    int err = sys_pub_json_encode(&sys, will_message_str, PAYLOAD_SIZE_MAX);
    if (err != 0)
    {
        LOG_ERR("Failed to encode sys pub json, err: %d", err);
        return err;
    }
    will_message.utf8 = will_message_str;
    will_message.size = strlen(will_message.utf8);

    LOG_HEXDUMP_DBG(will_topic.topic.utf8, will_topic.topic.size, "lwt topic");
    LOG_HEXDUMP_DBG(will_message.utf8, will_message.size, "lwt message");

    return 0;
}
struct mqtt_topic *lwt_topic_get(void)
{
    return &will_topic;
}
struct mqtt_utf8 *lwt_message_get(void)
{
    return &will_message;
}
