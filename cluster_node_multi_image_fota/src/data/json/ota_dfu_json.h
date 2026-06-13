#ifndef OTA_DFU_JSON_H
#define OTA_DFU_JSON_H
#include "ota_dfu_types.h"

/* module to encode and decode mqtt ota_dfu JSON payloads */

/*
    CMD TOPIC
    mq

    CMD PAYLOAD
{
    "session_id" : 101,
    "res_topic" : "ota_dfu_res",
    "uri":"http://10.30.224.10:8080",
    "file" : "app_update.bin",
    "retries" : 3,
    "install": "now"
}
    example string:
    {"session_id":103,"res_topic":"cmd/hs-nrf5340/00180700BA72/ota_dfu_res","uri":"http://10.0.0.200","file":"app_update_0.8.5.bin","retries":3,"install":"now"}

    CMD RESPONSE TOPIC
    cmd/hs-esp32s2/[mac]/ota_dfu_resp

    CMD RESPONSE PAYLOAD
{
    "session_id" : 99,
    "status": "success"
}

*/

int ota_dfu_json_cmd_decode(char *json_str, ota_dfu_cmd_t *ota_dfu_cmd);
int ota_dfu_json_cmd_resp_encode(ota_dfu_cmd_resp_t *ota_dfu_cmd_resp, char *json_str, int json_str_size);
void log_ota_dfu_cmd(ota_dfu_cmd_t *ota_dfu_cmd);
void log_ota_dfu_cmd_resp(ota_dfu_cmd_resp_t *ota_dfu_cmd_resp);

#define PAYLOAD_DECODE ota_dfu_json_cmd_decode
#define RSP_ENCODE ota_dfu_json_cmd_resp_encode
#define OTA_DFU_RESP_PAYLOAD_STR_SIZE_MAX 256

#define OTA_DFU_STATUS_PAYLOAD_STR_SIZE_MAX 1024
#define OTA_DFU_STATUS_PAYLOAD_ENCODE ota_dfu_status_json_encode
int ota_dfu_status_json_encode(ota_dfu_status_t *status, char *buf, int buf_len);

// optional field defaults
#define RES_QOS_DEFAULT 0

#endif // OTA_DFU_JSON_H
