#ifndef TEMPLATE_JSON_H
#define TEMPLATE_JSON_H
#include "template_sub_types.h"

/* module to encode and decode mqtt template JSON payloads */

/*
    CMD TOPIC
    cmd/hs-esp32s2/[mac]/template

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "template_res",
    "data" : 1234,
}
    CMD RESPONSE TOPIC
    cmd/hs-esp32s2/[mac]/template_resp

    CMD RESPONSE PAYLOAD
{
    "session_id" : 99,
    "status": "success"
}

*/

int template_json_cmd_decode(char *json_str, template_cmd_t *template_cmd);
int template_json_cmd_resp_encode(template_cmd_resp_t *template_cmd_resp, char *json_str, int json_str_size);
void log_template_cmd(template_cmd_t *template_cmd);
void log_template_cmd_resp(template_cmd_resp_t *template_cmd_resp);

#define PAYLOAD_DECODE template_json_cmd_decode
#define RSP_ENCODE template_json_cmd_resp_encode
#define TEMPLATE_RESP_PAYLOAD_STR_SIZE_MAX 256
#define OPTIONAL_FIELD_DEFAULT 0

#endif // TEMPLATE_JSON_H
