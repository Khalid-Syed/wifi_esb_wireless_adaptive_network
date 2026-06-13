#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include "mqtt_helper_heliogen.h"
#include "mqtt/mqtt_common.h"
#include "cfg.h"
#include "transport.h"
#include "transport_priv.h"
#include "lwt_pub_mqtt.h"

/* Declare log module */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(transport, CONFIG_MQTT_TRANSPORT_LOG_LEVEL);

/* Forward declarations */

// SFM state functions
static void network_disconnected_entry(void *o);
static void network_disconnected_run(void *o);

static void connecting_to_broker_entry(void *o);
static void connecting_to_broker_run(void *o);

static void connected_subscribing_entry(void *o);
static void connected_subscribing_run(void *o);
static void connected_subscribing_exit(void *o);

static void connected_publishing_entry(void *o);
static void connected_publishing_run(void *o);
static void connected_publishing_exit(void *o);

// work functions
static void connect_work_fn(struct k_work *work);
static void subscribe_work_fn(struct k_work *work);

// mqtt msg publish
static int msg_publish(transport_msg_t msg);
static void transport_fsm_timer_expired(void);

// service transport queue
static void service_transport_msg_queue(void);

/* Construct state machine table */
static const struct smf_state state[] = {
    [MQTT_NETWORK_DISCONNECTED] = SMF_CREATE_STATE(network_disconnected_entry, network_disconnected_run, NULL),
    [MQTT_CONNECTING_TO_BROKER] = SMF_CREATE_STATE(connecting_to_broker_entry, connecting_to_broker_run, NULL),
    [MQTT_CONNECTED_SUBSCRIBING] = SMF_CREATE_STATE(connected_subscribing_entry, connected_subscribing_run, connected_subscribing_exit),
    [MQTT_CONNECTED_PUBLISHING] = SMF_CREATE_STATE(connected_publishing_entry, connected_publishing_run, connected_publishing_exit),
};

/* Define connection work - Used to handle reconnection attempts to the MQTT broker */
static K_WORK_DELAYABLE_DEFINE(connect_work, connect_work_fn);

/* Define subscription work - Used to handle resubscription attempts to the MQTT broker */
static K_WORK_DELAYABLE_DEFINE(subscribe_work, subscribe_work_fn);

/* Define stack_area of application workqueue */
K_THREAD_STACK_DEFINE(stack_area, CONFIG_MQTT_TRANSPORT_WORKQUEUE_STACK_SIZE);

/* Declare application workqueue. This workqueue is used to call mqtt_helper_connect(), and
 * schedule reconnectionn attempts upon network loss or disconnection from MQTT.
 */
static struct k_work_q transport_work_queue;

static volatile bool is_mqtt_active = true;

/* User defined state object.
 * Used to transfer data between state changes.
 */
static struct s_object
{
    /* This must be first */
    struct smf_ctx ctx;
    /* Network event */
    enum network_events network_event;
    /* MQTT status */
    enum mqtt_events mqtt_event;
    /* Last Msg Id */
    uint16_t last_message_id;
} s_obj;

/**
 * @brief initialize transport fsm
 *
 */
void transport_fsm_init(void)
{
    LOG_DBG("");

    /* Initialize and start application workqueue.
     * This workqueue can be used to offload tasks and/or as a timer when wanting to
     * schedule functionality using the 'k_work' API.
     */
    k_work_queue_init(&transport_work_queue);
    k_work_queue_start(&transport_work_queue, stack_area,
                       K_THREAD_STACK_SIZEOF(stack_area),
                       K_HIGHEST_APPLICATION_THREAD_PRIO,
                       NULL);
    /* Set initial state */
    smf_set_initial(SMF_CTX(&s_obj), &state[MQTT_NETWORK_DISCONNECTED]);
}
/**
 * @brief run transport fsm
 *
 * @return int
 */
int transport_fsm_run(void)
{
    if (is_mqtt_active)
    {
        return smf_run_state(SMF_CTX(&s_obj));
    }
    else
    {
        return 1;
    }
}

/**
 * @brief change network event member of transport object
 *  This is called by network event handler
 * @param event
 */
void transport_mqtt_active_set(bool set_val)
{
    is_mqtt_active = set_val;
}

/**
 * @brief change network event member of transport object
 *  This is called by network event handler
 * @param event
 */
void transport_obj_network_event_set(int event)
{
    s_obj.network_event = event;
}
/**
 * @brief change mqtt event member of transport object
 *  This is called by mqtt event handler
 *
 * @param event
 */
void transport_obj_mqtt_event_set(int event)
{
    s_obj.mqtt_event = event;
}

/**
 * @brief Transport state is connected to mqtt broker and subscribed to topics, so ready to publish
 *
 * @return true
 * @return false
 */
bool transport_state_ready(void)
{
    if (s_obj.ctx.current == &state[MQTT_CONNECTED_PUBLISHING])
    {
        return true;
    }
    else
    {
        return false;
    }
}
/**
 * @brief change transport fsm state
 *
 * @param new_state
 */
void transport_state_change(enum module_state new_state)
{
    switch (new_state)
    {
    case MQTT_NETWORK_DISCONNECTED:
        smf_set_state(SMF_CTX(&s_obj), &state[MQTT_NETWORK_DISCONNECTED]);
        break;
    case MQTT_CONNECTING_TO_BROKER:
        smf_set_state(SMF_CTX(&s_obj), &state[MQTT_CONNECTING_TO_BROKER]);
        break;
    case MQTT_CONNECTED_SUBSCRIBING:
        smf_set_state(SMF_CTX(&s_obj), &state[MQTT_CONNECTED_SUBSCRIBING]);
        break;
    case MQTT_CONNECTED_PUBLISHING:
        smf_set_state(SMF_CTX(&s_obj), &state[MQTT_CONNECTED_PUBLISHING]);
        break;
    default:
        LOG_ERR("Unknown state");
        break;
    }
}

uint16_t transport_fsm_last_message_id_get(void)
{
    return s_obj.last_message_id;
}
void transport_fsm_last_message_id_set(uint16_t msg_id)
{
    s_obj.last_message_id = msg_id;
}
/*************************************************
 * Zephyr State Machine framework handlers
 * ***********************************************/

/* Function executed when the module enters the network disconnected state. */
static void network_disconnected_entry(void *o)
{
    struct s_object *user_object = o;
    // make sure the transport work is cancelled
    k_work_cancel_delayable(&connect_work);
    k_work_cancel_delayable(&subscribe_work);
    // Clear the transport queue to free any pending outgoing MQTT messages.
    // transport_queue_clear();
    // stop the transport pub timer
    transport_pub_timer_stop();
    user_object->network_event = NETWORK_NO_EVENT;  // clear any pending network events
    user_object->mqtt_event = MQTT_BROKER_NO_EVENT; // clear any pending mqtt events

    // send a "disconnected" event to the heliostat modules
    transport_status_event_throw(TRANSPORT_STATUS_EVENT_DISCONNECTED);
    // fsm will automatically transition to disconnected_run
}

/**
 * @brief Function executed when the module is in the disconnected state.
 * This waits for an IP connection to happen
 */
static void network_disconnected_run(void *o)
{
    struct s_object *user_object = o;

    switch (s_obj.network_event)
    {
    case NETWORK_NO_EVENT:
        break;
    case NETWORK_DISCONNECTED:
        // reset state so entry will be called again
        transport_state_change(MQTT_NETWORK_DISCONNECTED);
        break;
    case NETWORK_CONNECTED_WITH_IP:
        transport_state_change(MQTT_CONNECTING_TO_BROKER);
        break;
    }
    user_object->network_event = NETWORK_NO_EVENT;

    return;
}
/* Function executed when the module enters the connecting to broker state. */
static void connecting_to_broker_entry(void *o)
{
    ARG_UNUSED(o);

    // cancel any pending subscription work
    k_work_cancel_delayable(&subscribe_work);
    // Clear the transport queue to free any pending outgoing MQTT messages.
    transport_queue_clear();
    // stop the transport pub timer
    transport_pub_timer_stop();
    // signal to the heliostat modules that we are disconnected
    transport_status_event_throw(TRANSPORT_STATUS_EVENT_DISCONNECTED);

    /* Wait for 5 seconds to ensure that the network stack is ready before
     * attempting to connect to MQTT. This delay is only needed when building for
     * Wi-Fi.
     */
    k_work_reschedule_for_queue(&transport_work_queue, &connect_work, K_SECONDS(CONNECT_TO_BROKER_DELAY_SECS));
    // state will automatically transition to connecting_to_broker_run
}
static void connecting_to_broker_run(void *o)
{
    struct s_object *user_object = o;
    // look at network event first in case we are disconnected
    switch (user_object->network_event)
    {
    case NETWORK_NO_EVENT:
        break;
    case NETWORK_CONNECTED_WITH_IP:
        LOG_ERR("%s Unexpected event %s", __func__, network_evt_type_to_str(user_object->network_event));
        user_object->mqtt_event = MQTT_BROKER_NO_EVENT;    // dont process mqtt events
        transport_state_change(MQTT_CONNECTING_TO_BROKER); // go back to connecting to broker
        break;
    case NETWORK_DISCONNECTED:
        user_object->mqtt_event = MQTT_BROKER_NO_EVENT;    // dont process mqtt events
        transport_state_change(MQTT_NETWORK_DISCONNECTED); // go back to waiting for wifi
    default:
        break;
    }
    user_object->network_event = NETWORK_NO_EVENT;

    // if network is still connected, then look at mqtt events
    switch (user_object->mqtt_event)
    {
    case MQTT_BROKER_NO_EVENT:
        break;
    case MQTT_BROKER_DISCONNECTED:
        LOG_ERR("%s Unexpected event %s, reschedule broker connection", __func__, mqtt_evt_type_to_str(user_object->mqtt_event));
        k_work_reschedule_for_queue(&transport_work_queue, &connect_work, K_SECONDS(15));
        break;
    case MQTT_BROKER_CONNECT_TO_BROKER_FAIL:
        k_work_reschedule_for_queue(&transport_work_queue, &connect_work, K_SECONDS(15));
        break;
    case MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS: // yay
        LOG_DBG("");
        LOG_INF("Connected to MQTT broker");
        LOG_DBG("Client ID: %s", client_id());
        LOG_DBG("Port: %d", CONFIG_MQTT_HELPER_HELIOGEN_PORT);
        LOG_DBG("TLS: %s", IS_ENABLED(CONFIG_MQTT_LIB_TLS) ? "Yes" : "No");
        /* Cancel any ongoing connect work when we enter subscribing state */
        k_work_cancel_delayable(&connect_work);
        transport_state_change(MQTT_CONNECTED_SUBSCRIBING);
        break;
    case MQTT_BROKER_SUBSCRIBE_FAIL:
    case MQTT_BROKER_SUBSCRIBE_ACK: // try to reconnect
        LOG_ERR("%s Unexpected event %s", __func__, mqtt_evt_type_to_str(s_obj.mqtt_event));
        k_work_reschedule_for_queue(&transport_work_queue, &connect_work, K_SECONDS(15));
        break;
    default:
        break;
    }
    user_object->mqtt_event = MQTT_BROKER_NO_EVENT;
}

/* Function executed when the module enters the connected subscribing state. */
static void connected_subscribing_entry(void *o)
{
    ARG_UNUSED(o);
    LOG_DBG("Subscribing to default topics...");
    // subscribe to predefined topics;
    k_work_reschedule_for_queue(&transport_work_queue, &subscribe_work, K_SECONDS(5));
}
/* Function executed when the module is in the connected subscribing state. */
static void connected_subscribing_run(void *o)
{
    struct s_object *user_object = o;

    // look at network event first
    switch (user_object->network_event)
    {
    case NETWORK_NO_EVENT:
        break;
    case NETWORK_CONNECTED_WITH_IP:
        LOG_ERR("%s Unexpected event %s", __func__, network_evt_type_to_str(s_obj.network_event));
        user_object->mqtt_event = MQTT_BROKER_NO_EVENT;    // dont process mqtt events
        transport_state_change(MQTT_CONNECTING_TO_BROKER); // go back to connecting to broker
        break;
    case NETWORK_DISCONNECTED:
        user_object->network_event = NETWORK_NO_EVENT;
        user_object->mqtt_event = MQTT_BROKER_NO_EVENT; // dont process mqtt events
        /* Explicitly disconnect the MQTT transport when losing network connectivity.
         * This is to cleanup any internal library state.
         * The call to this function will cause on_mqtt_disconnect() to be called.
         */
        (void)mqtt_helper_disconnect();
        transport_state_change(MQTT_NETWORK_DISCONNECTED);
    default:
        break;
    }
    user_object->network_event = NETWORK_NO_EVENT;

    // if network is connected, then look at mqtt events
    switch (user_object->mqtt_event)
    {
    case MQTT_BROKER_NO_EVENT:
        break;
    case MQTT_BROKER_DISCONNECTED: // lost connection to broker
        LOG_DBG("MQTT_BROKER_DISCONNECTED");
        transport_state_change(MQTT_CONNECTING_TO_BROKER);
        break;
    case MQTT_BROKER_CONNECT_TO_BROKER_FAIL:
        LOG_ERR("%s Unexpected event %s", __func__, mqtt_evt_type_to_str(user_object->mqtt_event));
        transport_state_change(MQTT_CONNECTING_TO_BROKER);
        break;
    case MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS:
        LOG_ERR("%s Unexpected event %s", __func__, mqtt_evt_type_to_str(user_object->mqtt_event));
        break;
    case MQTT_BROKER_SUBSCRIBE_FAIL:
        LOG_DBG("MQTT_BROKER_SUBSCRIBE_FAILED");
        k_work_reschedule_for_queue(&transport_work_queue, &subscribe_work, K_SECONDS(5));
        break;
    case MQTT_BROKER_SUBSCRIBE_ACK: // yay
        LOG_DBG("Subscribed to default topics");
        transport_state_change(MQTT_CONNECTED_PUBLISHING);
        break;
    default:
        break;
    }
    user_object->mqtt_event = MQTT_BROKER_NO_EVENT;
}
static void connected_subscribing_exit(void *o)
{
    ARG_UNUSED(o);
    k_work_cancel_delayable(&subscribe_work);
}

/* Function executed when the module enters the connected publishing state. */
static void connected_publishing_entry(void *o)
{
    ARG_UNUSED(o);
    // we've successfully subscribed to all topics, so we are ready to publish
    // throw event so mqtt common will publish topics that broker needs to know about,
    // for example, fwv and ota_dfu status
    transport_status_event_throw(TRANSPORT_STATUS_EVENT_READY);
}

/* Function executed when the module is in the connected publishing state. */
static void connected_publishing_run(void *o)
{
    struct s_object *user_object = o;
    bool service_msg_queue = true;

    // look at network event first
    switch (user_object->network_event)
    {
    case NETWORK_NO_EVENT:
        break;
    case NETWORK_CONNECTED_WITH_IP:
        LOG_ERR("%s Unexpected event %s, but going to ignore", __func__, network_evt_type_to_str(s_obj.network_event));
        // user_object->mqtt_event = MQTT_BROKER_NO_EVENT; // dont process mqtt events
        // k_work_reschedule_for_queue(&transport_work_queue, &subscribe_work, K_SECONDS(15));
        // service_msg_queue = false;
        break;
    case NETWORK_DISCONNECTED:
        user_object->mqtt_event = MQTT_BROKER_NO_EVENT; // dont process mqtt events
        /* Explicitly disconnect the MQTT transport when losing network connectivity.
         * This is to cleanup any internal library state.
         * The call to this function will cause on_mqtt_disconnect() to be called.
         */
        (void)mqtt_helper_disconnect();
        transport_state_change(MQTT_NETWORK_DISCONNECTED);
        service_msg_queue = false;
        break;
    default:
        break;
    }
    user_object->network_event = NETWORK_NO_EVENT;

    // check mqtt events
    switch (user_object->mqtt_event)
    {
    case MQTT_BROKER_NO_EVENT:
        break;
    case MQTT_BROKER_DISCONNECTED: // lost connection to broker
        transport_state_change(MQTT_CONNECTING_TO_BROKER);
        service_msg_queue = false;
        break;
    case MQTT_BROKER_SUBSCRIBE_ACK:
        // play it safe and reconnect to broker
        LOG_ERR("%s Unexpected event %s but going to ignore", __func__, mqtt_evt_type_to_str(s_obj.mqtt_event));
        // transport_state_change(MQTT_CONNECTING_TO_BROKER);
        // service_msg_queue = false;
        break;
    case MQTT_BROKER_CONNECT_TO_BROKER_FAIL:
    case MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS:
    case MQTT_BROKER_SUBSCRIBE_FAIL:
        // play it safe and reconnect to broker
        LOG_ERR("%s Unexpected event %s", __func__, mqtt_evt_type_to_str(s_obj.mqtt_event));
        transport_state_change(MQTT_CONNECTING_TO_BROKER);
        service_msg_queue = false;
        break;
    default:
        break;
    }
    s_obj.mqtt_event = MQTT_BROKER_NO_EVENT;

    // if still connected to broker, then look at publishing queue
    if (service_msg_queue)
    {
        service_transport_msg_queue();
    }
}
static void connected_publishing_exit(void *o)
{
    ARG_UNUSED(o);
    // Clear the transport queue to free any pending outgoing MQTT messages.
    // transport_queue_clear();
    // stop the transport pub timer
    transport_pub_timer_stop();

    LOG_ERR("Disconnected from MQTT broker");
}

/**
 * @brief Service the transport message queue.
 *
 */
static void service_transport_msg_queue(void)
{
    /* look at outgoing publishing msg queue, send one if there is one to send and timer is not running */
    if ((transport_queue_pending() > 0) && (transport_pub_timer_running() == false))
    {
        transport_msg_t msg;
        if (transport_queue_peek(&msg) == 0) // is there a topic to publish?
        {
            // send it
            if (msg_publish(msg))
            {
                LOG_ERR("Failed to publish message to MQTT broker");
                transport_queue_remove(); // then remove it from the queue
            }
            else if (msg.qos == 0) // if qos 0, then remove it from the queue
            {
                transport_queue_remove();
            }
            else // if qos 1 or 2, set a timer to wait for puback
            {
                transport_pub_timer_start(transport_fsm_timer_expired);
            }
        }
    }
}
/*************************************************
 * mqtt message delivery and timer callbacks
 * ***********************************************/

/**
 * @brief callback when a qos 1 message is published
 *
 * @param msg_id
 * @param result
 */
void transport_fsm_msg_published(uint16_t msg_id, int result)
{
    ARG_UNUSED(msg_id);
    ARG_UNUSED(result);

    LOG_DBG("");
    transport_pub_timer_stop();
    transport_queue_remove();
}
/**
 * @brief callback when timer expired while wating for qos 1 puback
 *
 */
static void transport_fsm_timer_expired(void)
{
    LOG_DBG("qos 1 timer expired, remove msg from queue");
    transport_pub_timer_stop();
    transport_queue_remove();
}

/*************************************************
 * WORK HANDLERS
 * ***********************************************/

struct mqtt_topic will_topic = {
    .topic = {.utf8 = "i_am_dead",
              .size = 9},
    .qos = MQTT_QOS_1_AT_LEAST_ONCE};

struct mqtt_utf8 will_message = {
    .utf8 = "send_my_regards",
    .size = 15};

/* Connect work - Used to establish a connection to the MQTT broker and schedule reconnection
 * attempts.
 */
static void connect_work_fn(struct k_work *work)
{
    ARG_UNUSED(work);

    int err;
    struct mqtt_helper_conn_params conn_params;

    if (cfg_get_mqtt_hostname() != NULL)
    {
        conn_params.hostname.ptr = cfg_get_mqtt_hostname();
        conn_params.hostname.size = strlen(cfg_get_mqtt_hostname());
        conn_params.device_id.ptr = client_id();
        conn_params.device_id.size = client_id_len();
        conn_params.user_name.ptr = "";
        conn_params.user_name.size = 0;
    }
    else
    {
        conn_params.hostname.ptr = CONFIG_MQTT_TRANSPORT_BROKER_HOSTNAME;
        conn_params.hostname.size = strlen(CONFIG_MQTT_TRANSPORT_BROKER_HOSTNAME);
        conn_params.device_id.ptr = client_id();
        conn_params.device_id.size = client_id_len();
        conn_params.user_name.ptr = "";
        conn_params.user_name.size = 0;
    }

    // create last will and testiment mqtt message
    // this will be sent to the broker when connecting, and if a disconnect is detected by the broker,
    // the LWT topic/message will be published by the broker to all subscribed clients.
    // This will let them know that the node has disconnected unexpectedly.
    if (lwt_create() != 0)
    {
        LOG_ERR("Failed to create last will and testament");
    }

    k_sleep(K_MSEC(10000));

    // connect to broker
    err = mqtt_helper_connect(&conn_params, lwt_topic_get(), lwt_message_get());
    if (err)
    {
        if (err == -EOPNOTSUPP)
        {
            LOG_ERR("Failed connecting to MQTT, error code: -EOPNOTSUPP, lib is in wrong state, "
                    "so explicitly disconnect, then retry connect in %d seconds",
                    CONFIG_MQTT_TRANSPORT_RECONNECTION_TIMEOUT_SECONDS);
            mqtt_helper_disconnect();
        }
        else
        {
            LOG_INF("Failed connecting to MQTT, error code: %d, retry in %d seconds", err, CONFIG_MQTT_TRANSPORT_RECONNECTION_TIMEOUT_SECONDS);
        }
    }

    k_work_reschedule_for_queue(&transport_work_queue, &connect_work,
                                K_SECONDS(CONFIG_MQTT_TRANSPORT_RECONNECTION_TIMEOUT_SECONDS));
}

/* Subscribe work - Used to establish a subscriptions to the MQTT broker and schedule resubscribe
 * attempts.
 */
static void subscribe_work_fn(struct k_work *work)
{
    ARG_UNUSED(work);

    // subscribe to predefined topics;
    mqtt_common_default_topics_subscribe();
}

/*************************************************
 * END WORK HANDLERS
 * ***********************************************/

/**
 * @brief Function to publish a message to the MQTT broker.
 *
 * @param msg
 * @return int
 */
static int msg_publish(transport_msg_t msg)
{
    int err;
    struct mqtt_publish_param param =
        {
            .message.payload.data = msg.payload,
            .message.payload.len = strlen(msg.payload),
            .message.topic.qos = msg.qos,
            .message.topic.topic.utf8 = msg.topic,
            .message.topic.topic.size = strlen(msg.topic),
            .message_id = k_uptime_get_32(),
            .retain_flag = 1,
        };

    err = mqtt_helper_publish(&param);
    if (err)
    {
        LOG_WRN("Failed to send payload, err: %d", err);
        return -1;
    }

    LOG_DBG("Published message: \"%.*s\" on topic: \"%.*s\" with qos %d", param.message.payload.len,
            param.message.payload.data,
            param.message.topic.topic.size,
            param.message.topic.topic.utf8,
            param.message.topic.qos);

    return 0;
}

/**
 * @brief Transport state string get
 *
 * @return const char*
 */
const char *transport_fsm_state_to_str(void)
{
    if (s_obj.ctx.current == &state[MQTT_NETWORK_DISCONNECTED])
    {
        return "MQTT_NETWORK_DISCONNECTED";
    }
    else if (s_obj.ctx.current == &state[MQTT_CONNECTING_TO_BROKER])
    {
        return "MQTT_CONNECTING_TO_BROKER";
    }
    else if (s_obj.ctx.current == &state[MQTT_CONNECTED_SUBSCRIBING])
    {
        return "MQTT_CONNECTED_SUBSCRIBING";
    }
    else if (s_obj.ctx.current == &state[MQTT_CONNECTED_PUBLISHING])
    {
        return "MQTT_CONNECTED_PUBLISHING";
    }
    else
    {
        return "MQTT_STATE_UNKNOWN";
    }
}
const char *network_evt_type_to_str(enum network_events network_evt)
{
    switch (network_evt)
    {
    case NETWORK_NO_EVENT:
        return "NETWORK_NO_EVENT";
    case NETWORK_DISCONNECTED:
        return "NETWORK_DISCONNECTED";
    case NETWORK_CONNECTED_WITH_IP:
        return "NETWORK_CONNECTED_WITH_IP";
    default:
        return "NETWORK_EVT_TYPE_UNKNOWN";
    }
}
const char *mqtt_evt_type_to_str(enum mqtt_events mqtt_evt)
{
    switch (mqtt_evt)
    {
    case MQTT_BROKER_NO_EVENT:
        return "MQTT_BROKER_NO_EVENT";
    case MQTT_BROKER_DISCONNECTED:
        return "MQTT_BROKER_DISCONNECTED";
    case MQTT_BROKER_CONNECT_TO_BROKER_FAIL:
        return "MQTT_BROKER_CONNECT_TO_BROKER_FAIL";
    case MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS:
        return "MQTT_BROKER_CONNECT_TO_BROKER_SUCCESS";
    case MQTT_BROKER_SUBSCRIBE_FAIL:
        return "MQTT_BROKER_SUBSCRIBE_FAIL";
    case MQTT_BROKER_SUBSCRIBE_ACK:
        return "MQTT_BROKER_SUBSCRIBE_ACK";
    default:
        return "MQTT_EVT_TYPE_UNKNOWN";
    }
}