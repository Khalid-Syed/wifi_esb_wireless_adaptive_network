#ifndef __TRANSPORT_PRIV_H__
#define __TRANSPORT_PRIV_H__

#include <zephyr/kernel.h>
#include "mqtt_helper_heliogen.h"
#include "transport_events.h"
#include "transport_timer.h"
#include "transport_queue.h"
#include "null_param_check.h"

// private funcs
/* Internal states */
enum module_state
{
    MQTT_NETWORK_DISCONNECTED,
    MQTT_CONNECTING_TO_BROKER,
    MQTT_CONNECTED_SUBSCRIBING,
    MQTT_CONNECTED_PUBLISHING,
};
/* ip network events */
enum network_events
{
    NETWORK_NO_EVENT,
    NETWORK_DISCONNECTED,
    NETWORK_CONNECTED_WITH_IP,
};
enum mqtt_events
{
    MQTT_BROKER_NO_EVENT,
    MQTT_BROKER_DISCONNECTED,
    MQTT_BROKER_CONNECT_TO_BROKER_FAIL,
    MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS,
    MQTT_BROKER_SUBSCRIBE_ACK,
    MQTT_BROKER_SUBSCRIBE_FAIL,
    MQTT_BROKER_PUBLISH_ACK,
    MQTT_BROKER_PUBLISH_FAIL,
    MQTT_BROKER_RX_MSG,
    MQTT_BROKER_ERROR,
};

/* transport fsm accessors called by transport thread and mqtt callbacks */
void transport_fsm_init(void);
int transport_fsm_run(void);
void transport_obj_mqtt_event_set(int event);
void transport_obj_network_event_set(int event);
void transport_mqtt_active_set(bool set_val);

const char *transport_fsm_state_to_str(void);
const char *network_evt_type_to_str(enum network_events);
const char *mqtt_evt_type_to_str(enum mqtt_events);
uint16_t transport_fsm_last_message_id_get(void);
void transport_fsm_last_message_id_set(uint16_t msg_id);
void transport_fsm_msg_published(uint16_t msg_id, int result);
void transport_fsm_msg_timeout(void);

/* init mqtt helper callbacks */
void transport_mqtt_callbacks_init(struct mqtt_helper_cfg *cfg);

/* client id api */
int client_id_init(void);
char *client_id(void);
int client_id_len(void);

// stats event call
void transport_stats(enum mqtt_events evt);

#endif // __TRANSPORT_PRIV_H__
