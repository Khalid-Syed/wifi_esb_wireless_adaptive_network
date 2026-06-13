#ifndef START_JSON_H
#define START_JSON_H
#include "start_types.h"
/* module to encode and decode mqtt START JSON payloads */

/*
    CMD TOPIC
    cmd/hs-nrf5340/start

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "start_res",
    "operation" : "START"
}
    CMD RESPONSE TOPIC
    cmd/hs-nrf5340/start_res

    CMD RESPONSE PAYLOAD
{
    "session_id" : 99,
    "node_id" : "MAC ID" / "NA",
    "status": "success"
}

*/

int start_json_cmd_decode(char *json_str, start_cmd_t *start_cmd);
int start_json_cmd_resp_encode(start_cmd_resp_t *start_cmd_resp, char *json_str, int json_str_size);
void log_start_cmd(start_cmd_t *start_cmd);
void log_start_cmd_resp(start_cmd_resp_t *start_cmd_resp);

#define PAYLOAD_DECODE start_json_cmd_decode
#define RSP_ENCODE start_json_cmd_resp_encode
#define START_RESP_PAYLOAD_STR_SIZE_MAX 256
#define START_OPERATION_DEFAULT "NA"
#endif // START_JSON_H
