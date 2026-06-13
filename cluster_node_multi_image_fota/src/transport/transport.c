#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "mqtt_helper_heliogen.h"
#include "mqtt_common.h"
#include "transport.h"
#include "transport_priv.h"

/* Register log module */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(transport, CONFIG_MQTT_TRANSPORT_LOG_LEVEL);

#define TRANSPORT_LOOP_SLEEP_MS 50
#define TRANSPORT_THREAD_MONITOR_INTERVAL_MS 1000 // [G5FW-209] loosen thread monitor timing

/* Forward declarations */
static void transport_thread(void *, void *, void *);

/* Transport thread */
K_THREAD_STACK_DEFINE(transport_thread_stack_area, CONFIG_MQTT_TRANSPORT_THREAD_STACK_SIZE);
struct k_thread transport_thread_data;

static void (*im_alive_cb)(int, uint32_t, uint32_t); // callback to manager thread to indicate thread is healthy
static int thread_idx;								 // index to pass back to im_alive callback

/**
 * @brief
 *
 * @param cb - callback to indicate thread is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
k_tid_t transport_start(void(*cb), int idx)
{
	im_alive_cb = cb;
	thread_idx = idx;

	k_tid_t my_tid = k_thread_create(&transport_thread_data, transport_thread_stack_area,
									 CONFIG_MQTT_TRANSPORT_THREAD_STACK_SIZE,
									 transport_thread,
									 NULL, NULL, NULL,
									 CONFIG_MQTT_TRANSPORT_THREAD_PRIORITY, 0, K_NO_WAIT);
	return my_tid;
}
/**
 * @brief call this when transport is stalled or otherwise not healthy
 *
 */
void transport_stall_assert(void)
{
	__ASSERT(0, "transport thread stalled");
}
/**
 * @brief Transport subscribe
 *
 * @param topics
 * @param topic_count
 * @param message_id
 */
void transport_subscribe(struct mqtt_topic *topics, size_t topic_count, uint16_t message_id)
{
	int err;
	struct mqtt_subscription_list list = {
		.list = topics,
		.list_count = topic_count,
		.message_id = message_id,
	};
	err = mqtt_helper_subscribe(&list);
	if (err)
	{
		LOG_ERR("Failed to subscribe to topics, error: %d", err);
		return;
	}
	transport_fsm_last_message_id_set(message_id);
}
/**
 * @brief Transport publish
 *
 * @param topic
 * @param payload
 * @param qos
 * @return int
 */
int transport_publish(char *topic, char *payload, uint8_t qos)
{
	NULL_PARAM_CHECK(topic);
	NULL_PARAM_CHECK(payload);

	if (transport_state_ready())
	{
		transport_msg_t msg = {
			.topic = topic,
			.payload = payload,
			.qos = qos,
		};
		// don't allow qos 2, not supported without patch to mqtt_helper.c - see note below
		if ((qos > 1) || (qos < 0))
		{
			msg.qos = 1;
		}

		if (msg.topic == NULL || (strlen(msg.topic) >= CONFIG_MQTT_TOPIC_STRING_MAX_SIZE))
		{
			LOG_ERR("Topic string too long");
			return -1;
		}
		if (msg.payload == NULL || strlen(msg.payload) >= CONFIG_MQTT_PAYLOAD_STRING_MAX_SIZE)
		{
			LOG_ERR("Payload string too long");
			return -1;
		}
		LOG_DBG("Adding to Pub Queue - topic:payload %s %s", msg.topic, msg.payload);
		if (transport_queue_add(msg))
		{
			LOG_ERR("%s Failed to add to queue", __func__);
			return -1;
		}
	}
	else
	{
		LOG_DBG("Publish %s requested, but transport not connected", topic);
		return -1;
	}
	return 0;
}
const char *transport_state_str_get(void)
{
	return transport_fsm_state_to_str();
}
/********************/
/* private functions*/
/********************/

static volatile bool is_mqtt_active = true;

static void transport_thread(void *, void *, void *)
{
	int err;

	struct mqtt_helper_cfg cfg;

	client_id_init();
	transport_fsm_init();

	transport_mqtt_callbacks_init(&cfg);
	err = mqtt_helper_init(&cfg);
	if (err)
	{
		LOG_ERR("mqtt_helper_init, error: %d", err);
		im_alive_cb(thread_idx, 0, 0); // turn off the thread monitor for this thread
		// SEND_FATAL_ERROR();
		return;
	}

	while (1)
	{
		im_alive_cb(thread_idx, k_uptime_get_32(), TRANSPORT_THREAD_MONITOR_INTERVAL_MS);

		transport_fsm_run();

		k_sleep(K_MSEC(TRANSPORT_LOOP_SLEEP_MS));
	}
}

/*
QoS 2 Support Note:
Qo2 of two is "deliver only once", which is expensive transactionally, and not needed for our use case.

To support qos of 2, mqtt_helper.c needs this swtich statement case in:
MQTT_HELPER_STATIC void mqtt_evt_handler(struct mqtt_client *const mqtt_client,
				 const struct mqtt_evt *mqtt_evt)
 otherwise, there will be no callback to our layer to let us know that the message was delivered.
 The effort to support a patch is not worth it.

	case MQTT_EVT_PUBREC:
		LOG_DBG("MQTT_EVT_PUBREC");
		// call application pub ack callback if set (there is no cb handler for QoS pub received)
		if (current_cfg.cb.on_puback) {
			current_cfg.cb.on_puback(mqtt_evt->param.puback.message_id,
						 mqtt_evt->result);
		}
		break;

 *
 */