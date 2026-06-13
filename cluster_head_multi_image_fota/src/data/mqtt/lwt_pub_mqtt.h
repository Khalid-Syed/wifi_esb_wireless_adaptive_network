#ifndef LWT_PUB_MQTT_H
#define LWT_PUB_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "mqtt_common.h"

/* module to encode and decode mqtt LWT_PUB JSON payloads */

// last will and testiment functions
int lwt_create(void);
struct mqtt_topic *lwt_topic_get(void);
struct mqtt_utf8 *lwt_message_get(void);

#endif // LWT_PUB_MQTT_H
