#ifndef CONTROL_JSON_H
#define CONTROL_JSON_H
#include "control_types.h"
/* module to encode and decode mqtt CONTROL JSON payloads */

/*
    CMD TOPIC
    cmd/hs-nrf5340/[mac]/control

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "control_res",
    "operation" : "CLA" or "NCLA"
}
    CMD RESPONSE TOPIC
    cmd/hs-nrf5340/[mac]/control_res

    CMD RESPONSE PAYLOAD
{
    "session_id" : 99,
    "status": "success"
}

*/

int control_json_cmd_decode(char *json_str, control_cmd_t *control_cmd);
int control_json_cmd_resp_encode(control_cmd_resp_t *control_cmd_resp, char *json_str, int json_str_size);
void log_control_cmd(control_cmd_t *control_cmd);
void log_control_cmd_resp(control_cmd_resp_t *control_cmd_resp);

#define PAYLOAD_DECODE control_json_cmd_decode
#define RSP_ENCODE control_json_cmd_resp_encode
#define CONTROL_RESP_PAYLOAD_STR_SIZE_MAX 256
#define CONTROL_OPERATION_DEFAULT "NCLA"
#endif // CONTROL_JSON_H
