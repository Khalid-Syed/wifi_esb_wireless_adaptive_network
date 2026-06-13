#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

/* Public API of tranport module(s)*/

#include <zephyr/kernel.h>
#include "transport_events.h"
#include "transport_stats.h"

#define CONNECT_TO_BROKER_DELAY_SECS 10 // seconds to wait after wifi is connected and has ip to connecct to mqtt broker
/**
 * @brief
 *
 * @param cb - callback to indicate thread is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
k_tid_t transport_start(void(*cb), int idx);
/**
 * @brief call this when transport is stalled or otherwise not healthy
 *
 */
void transport_stall_assert(void);
/**
 * @brief get the transport state as a string
 *
 * @return const char*
 */
const char *transport_state_str_get(void);
/**
 * @brief get bool true if transport is connected and ready to publish
 *
 * @return true
 * @return false
 */
bool transport_state_ready(void);
/**
 * @brief Transport subscribe to a list of topics
 *
 * @param topics
 * @param topic_count
 * @param message_id
 */
void transport_subscribe(struct mqtt_topic *topics, size_t topic_count, uint16_t message_id);
/**
 * @brief Transport publish a topic and payload
 *
 * @param topic
 * @param payload
 * @param qos
 * @return int
 */
int transport_publish(char *topic, char *payload, uint8_t qos);
/**
 * @brief Transport subscribe to a list of default topics
 *
 */
void transport_subscribe_to_default_topics(void);

/**
 * @brief how many seconds transport has been connected to mqtt broker
 *
 * @return int
 */
int transport_connected_seconds(void);

#endif // __TRANSPORT_H__
