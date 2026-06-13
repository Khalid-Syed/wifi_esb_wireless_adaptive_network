#ifndef NODE_ID_JSON_H
#define NODE_ID_JSON_H
#include "node_id_types.h"
/* module to encode and decode mqtt NODE_ID JSON payloads */

/*
    CMD TOPIC
    cmd/hs-nrf5340/node_id

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "node_id_res",
    "operation" : "GET_NODE_ID"
}
    CMD RESPONSE TOPIC
    cmd/hs-nrf5340/node_id_res

    CMD RESPONSE PAYLOAD
{
    "node_id" : "MAC ID" / "NA"
}

*/

int node_id_json_cmd_decode(char *json_str, node_id_cmd_t *node_id_cmd);
int node_id_json_cmd_resp_encode(node_id_cmd_resp_t *node_id_cmd_resp, char *json_str, int json_str_size);
void log_node_id_cmd(node_id_cmd_t *node_id_cmd);
void log_node_id_cmd_resp(node_id_cmd_resp_t *node_id_cmd_resp);

#define PAYLOAD_DECODE node_id_json_cmd_decode
#define RSP_ENCODE node_id_json_cmd_resp_encode
#define NODE_ID_RESP_PAYLOAD_STR_SIZE_MAX 256
#define NODE_ID_OPERATION_DEFAULT "GET_NODE_ID"
#endif // NODE_ID_JSON_H
