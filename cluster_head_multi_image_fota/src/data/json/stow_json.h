#ifndef STOW_JSON_H
#define STOW_JSON_H
#include "stow_types.h"

/* module to encode and decode mqtt stow JSON payloads */

/*
    CMD TOPIC
    cmd/hs-esp32s2/[mac]/stow

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "stow_res",
    "retry" : 3,
}
    CMD RESPONSE TOPIC
    cmd/hs-esp32s2/[mac]/stow_resp

    CMD RESPONSE PAYLOAD
{
    "session_id" : 99,
    "status": "ok"
}

*/

int stow_json_cmd_decode(char *json_str, stow_cmd_t *stow_cmd);
int stow_json_cmd_resp_encode(stow_cmd_resp_t *stow_cmd_resp, char *json_str, int json_str_size);
void log_stow_cmd(stow_cmd_t *stow_cmd);
void log_stow_cmd_resp(stow_cmd_resp_t *stow_cmd_resp);

#define PAYLOAD_DECODE stow_json_cmd_decode
#define RSP_ENCODE stow_json_cmd_resp_encode
#define STOW_RESP_PAYLOAD_STR_SIZE_MAX 256
#endif // STOW_JSON_H
