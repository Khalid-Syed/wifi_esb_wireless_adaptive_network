#ifndef OTA_DFU_MQTT_H
#define OTA_DFU_MQTT_H
#include "mqtt_helper_heliogen.h"
#include "ota_dfu_types.h"

/* module to encode and decode mqtt ota dfu JSON payloads */

/*
    CMD TOPIC
    cmd/[platform]/[mac]/ota_dfu

    CMD TOPIC RESPONSE
    cmd/[platform]/[mac]/ota_dfu_resp

    DT TOPIC STATUS
    dt/[platform]/[mac]/ota_dfu_status
*/

#define OTA_DFU_MQTT_TOPIC_CMD_STR "cmd/" MQTT_PLATFORM "/%s/ota_dfu"
#define OTA_DFU_MQTT_TOPIC_CMD_RSP_STR "cmd/" MQTT_PLATFORM "/%s/%s"
#define OTA_DFU_MQTT_CMD_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define OTA_DFU_MQTT_TOPIC_CMD_ERR_RSP_STR "cmd/" MQTT_PLATFORM "/%s/ota_dfu_res"
#define OTA_DFU_MQTT_CMD_RSP_QOS MQTT_QOS_1_AT_LEAST_ONCE

#define OTA_DFU_PUB_TOPIC "ota_dfu_status"
#define OTA_DFU_STATUS_MQTT_TOPIC_DT_STR "dt/" MQTT_PLATFORM "/%s/ota_dfu_status"
#define OTA_DFU_STATUS_MQTT_DT_QOS MQTT_QOS_1_AT_LEAST_ONCE

int ota_dfu_mqtt_topic_get(struct mqtt_topic *topic);
bool ota_dfu_on_mqtt_publish_cb(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload);

int ota_dfu_status_mqtt_topic_get(struct mqtt_topic *topic, char *topic_str, size_t topic_str_len);
int ota_dfu_status_mqtt_publish(ota_dfu_status_t *status);
#endif // GOTO_MQTT_H

/*
Example ota_dfu MQTT Topic and JSON Payload

cmd/hs-nrf5340/001807002172/ota_dfu

{
  "session_id": 100,
  "res_topic": "ota_dfu_resp",
  "uri": "http://10.0.0.200",
  "file": "app_update_0.5.6.bin",
  "install": "now",
  "retries": 3
}
*/