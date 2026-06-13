#ifndef REBOOT_JSON_H
#define REBOOT_JSON_H
#include "reboot_types.h"
/* module to encode and decode mqtt REBOOT JSON payloads */

/*
    CMD TOPIC
    cmd/hs-nrf5340/[mac]/reboot

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "reboot_res",
    "delay" : 0
}
    CMD RESPONSE TOPIC
    cmd/hs-nrf5340/[mac]/reboot_resp

    CMD RESPONSE PAYLOAD
{
    "session_id" : 99,
    "status": "success"
}

*/

int reboot_json_cmd_decode(char *json_str, reboot_cmd_t *reboot_cmd);
int reboot_json_cmd_resp_encode(reboot_cmd_resp_t *reboot_cmd_resp, char *json_str, int json_str_size);
void log_reboot_cmd(reboot_cmd_t *reboot_cmd);
void log_reboot_cmd_resp(reboot_cmd_resp_t *reboot_cmd_resp);

#define PAYLOAD_DECODE reboot_json_cmd_decode
#define RSP_ENCODE reboot_json_cmd_resp_encode
#define REBOOT_RESP_PAYLOAD_STR_SIZE_MAX 256
#define REBOOT_DELAY_DEFAULT 0
#endif // REBOOT_JSON_H
