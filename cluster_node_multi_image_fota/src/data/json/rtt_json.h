#ifndef RTT_JSON_H
#define RTT_JSON_H
#include "rtt_types.h"
/* module to encode and decode mqtt RTT JSON payloads */

/*
    CMD TOPIC
    cmd/proton-node/rtt

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "rtt_res",
    "operation" : "RTT",
    "payload" : "command payload"
}
    CMD RESPONSE TOPIC
    cmd/hs-nrf5340/rtt_res

    CMD RESPONSE PAYLOAD
{
    "node_id" : "MAC ID" / "NA"
    "latest_ipc_msg" : "latest received IPC message" / "NA"
}

*/

int rtt_json_cmd_decode(char *json_str, rtt_cmd_t *rtt_cmd);
int rtt_json_cmd_resp_encode(rtt_cmd_resp_t *rtt_cmd_resp, char *json_str, int json_str_size);
void log_rtt_cmd(rtt_cmd_t *rtt_cmd);
void log_rtt_cmd_resp(rtt_cmd_resp_t *rtt_cmd_resp);

#define PAYLOAD_DECODE rtt_json_cmd_decode
#define RSP_ENCODE rtt_json_cmd_resp_encode
#define RTT_RESP_PAYLOAD_STR_SIZE_MAX 256
#define RTT_RES_TOPIC_DEFAULT "cmd/proton-node/rtt_res"
#define RTT_OPERATION_DEFAULT "RTT"
#endif // RTT_JSON_H
