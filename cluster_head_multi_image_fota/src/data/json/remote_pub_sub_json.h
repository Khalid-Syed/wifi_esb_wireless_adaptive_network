#ifndef REMOTE_PUB_JSON_H
#define REMOTE_PUB_JSON_H
#include "remote_pub_sub_types.h"

/* module to encode and decode mqtt remote_pub JSON payloads */

/*
    CMD TOPIC
    cmd/hs-nrf5340/[mac]/remote_pub

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "remote_pub_res",
    "topic" : "status",
    "delay_ms" : 1234,
}
    CMD RESPONSE TOPIC
    cmd/hs-nrf5340/[mac]/remote_pub_resp

    CMD RESPONSE PAYLOAD
{
    "session_id" : 99,
    "status": "error-not-found",
}

*/

int remote_pub_json_cmd_decode(char *json_str, remote_pub_cmd_t *remote_pub_cmd);
int remote_pub_json_cmd_resp_encode(remote_pub_cmd_resp_t *remote_pub_cmd_resp, char *json_str, int json_str_size);
void log_remote_pub_cmd(remote_pub_cmd_t *remote_pub_cmd);
void log_remote_pub_cmd_resp(remote_pub_cmd_resp_t *remote_pub_cmd_resp);

#define PAYLOAD_DECODE remote_pub_json_cmd_decode
#define RSP_ENCODE remote_pub_json_cmd_resp_encode
#define REMOTE_PUB_RESP_PAYLOAD_STR_SIZE_MAX 256
#define OPTIONAL_FIELD_DEFAULT 0

#endif // REMOTE_PUB_JSON_H
