
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include "mqtt_helper_heliogen.h"

#include "../data/mqtt/mqtt_common.h"
#include "transport_priv.h"

/* Register log module */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(transport, CONFIG_MQTT_TRANSPORT_LOG_LEVEL);

/* Callback handlers from MQTT helper library.
 * The functions are called whenever specific MQTT packets are received from the broker, or
 * some library state has changed.
 */
static void on_mqtt_connack(enum mqtt_conn_return_code return_code);
static void on_mqtt_disconnect(int result);
static void on_mqtt_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);
static void on_mqtt_puback(uint16_t msg_id, int result);
static void on_mqtt_suback(uint16_t message_id, int result);
static void on_mqtt_pingresp(void);
static void on_mqtt_error(enum mqtt_helper_error err);

/********************/
/* public functions*/
/********************/

/**
 * @brief Initialize MQTT helper callbacks
 *
 * @param cfg
 */
void transport_mqtt_callbacks_init(struct mqtt_helper_cfg *cfg)
{
    cfg->cb.on_connack = on_mqtt_connack;
    cfg->cb.on_disconnect = on_mqtt_disconnect;
    cfg->cb.on_publish = on_mqtt_publish;
    cfg->cb.on_puback = on_mqtt_puback;
    cfg->cb.on_suback = on_mqtt_suback;
    cfg->cb.on_pingresp = on_mqtt_pingresp;
    cfg->cb.on_error = on_mqtt_error;
}
/********************/
/* private functions*/
/********************/

/* Callback handlers from MQTT helper library.
 * The functions are called whenever specific MQTT packets are received from the broker, or
 * some library state has changed.
 */
static void on_mqtt_connack(enum mqtt_conn_return_code return_code)
{
    ARG_UNUSED(return_code);
    if (return_code == MQTT_CONNECTION_ACCEPTED)
    {

        LOG_INF("MQTT client connected!");
        transport_stats(MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS); // track statistics
        transport_obj_mqtt_event_set(MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS);
    }
    else
    {
        LOG_ERR("MQTT client connection refused!");
        transport_stats(MQTT_BROKER_CONNECT_TO_BROKER_FAIL); // track statistics
        transport_obj_mqtt_event_set(MQTT_BROKER_CONNECT_TO_BROKER_FAIL);
        return;
    }
}

static void on_mqtt_disconnect(int result)
{
    LOG_ERR("MQTT client disconnected! result: %d", result);
    transport_stats(MQTT_BROKER_DISCONNECTED); // track statistics
    transport_obj_mqtt_event_set(MQTT_BROKER_DISCONNECTED);
}

static void on_mqtt_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
    LOG_DBG("Received payload: \"%.*s\" on topic: \"%.*s\"", payload.size,
            payload.ptr,
            topic.size,
            topic.ptr);
    transport_stats(MQTT_BROKER_RX_MSG); // track statistics
    if (!mqtt_common_on_mqtt_publish(topic, payload))
    {
        LOG_WRN("%s No handler for topic: \"%.*s\"", __func__, topic.size, topic.ptr);
    }
}
static void on_mqtt_puback(uint16_t msg_id, int result)
{
    LOG_DBG("PubAck: msgid %d result %d", msg_id, result);
    if (result == 0)
    {
        transport_stats(MQTT_BROKER_PUBLISH_ACK); // track statistics
    }
    else
    {
        transport_stats(MQTT_BROKER_PUBLISH_FAIL);
    }
    transport_fsm_msg_published(msg_id, result);
}

static void on_mqtt_suback(uint16_t message_id, int result)
{
    LOG_DBG("SubAck: msgid %d result %d", message_id, result);
    if ((message_id == transport_fsm_last_message_id_get()) && (result == 0))
    {
        LOG_DBG("Subscribed to topic with msg id %d", transport_fsm_last_message_id_get());
        transport_stats(MQTT_BROKER_SUBSCRIBE_ACK); // track statistics
        transport_obj_mqtt_event_set(MQTT_BROKER_SUBSCRIBE_ACK);
    }
    else if (result)
    {
        transport_stats(MQTT_BROKER_SUBSCRIBE_FAIL); // track statistics
        transport_obj_mqtt_event_set(MQTT_BROKER_SUBSCRIBE_FAIL);
    }
    else
    {
        LOG_WRN("Subscribed to unknown topic, id: %d", message_id);
    }
}
static void on_mqtt_pingresp(void)
{
    LOG_DBG("MQTT Ping response received");
}
static void on_mqtt_error(enum mqtt_helper_error err)
{
    transport_stats(MQTT_BROKER_ERROR); // track statistics
    LOG_ERR("MQTT error received - %d MQTT_HELPER_ERROR_MSG_SIZE", err);
}
