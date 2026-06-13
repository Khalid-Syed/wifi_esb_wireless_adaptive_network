#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_common.h"
#include "template_pub_mqtt.h"
#include "transport.h"
#include "id.h"
#include "transport_events.h"
#include "template_pub_json.h" // this would be switched out for protobuf or other as needed

#define MODULE template_pub_mqtt
LOG_MODULE_REGISTER(MODULE, CONFIG_TEMPLATE_PUB_MQTT_LOG_LEVEL);

#define MQTT_TOPIC_GET template_pub_mqtt_topic_get
#define MQTT_PUBLISH template_pub_mqtt_publish
// dt topic
#define TOPIC TEMPLATE_PUB_MQTT_TOPIC_DT_STR
#define TOPIC_DT_LEN ((sizeof(TEMPLATE_PUB_MQTT_TOPIC_DT_STR) + ID_LEN) + 1)
#define QOS_DT TEMPLATE_PUB_MQTT_DT_QOS

#define DT_OBJ_TYPE template_pub_t
#define DT_OBJ_P_TYPE DT_OBJ_TYPE *

#define PAYLOAD_SIZE_MAX TEMPLATE_PUB_PAYLOAD_STR_SIZE_MAX

static bool transport_ready = false;

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
    if (!transport_ready)
        return -1;

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
/**
 * @brief callback when transport is ready for publishing
 *
 * @param aeh
 * @return true
 * @return false
 */
static bool template_transport_event_handler(const struct app_event_header *aeh)
{
    LOG_DBG("%s", __func__);

    if (is_transport_status_event(aeh))
    {
        struct transport_status_event *le = cast_transport_status_event(aeh);
        LOG_DBG("transport status event: %s", le->status);
        if (!strcmp(le->status, TRANSPORT_STATUS_EVENT_READY))
        {
            transport_ready = true;
            // if applicable, send initial pub message
            MQTT_PUBLISH(&template_pub);
        }
        else if (!strcmp(le->status, TRANSPORT_STATUS_EVENT_DISCONNECTED))
        {
            // stop publishing messages
            transport_ready = false;
        }
        else
        { // unhandled events
            LOG_DBG("unknown transport status event: %s", le->status);
        }
        return false;
    }

    /* If event is unhandled, unsubscribe. */
    __ASSERT_NO_MSG(false);
    return false;
}

// subscribe to transport events
APP_EVENT_LISTENER(MODULE, template_transport_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, transport_status_event);
