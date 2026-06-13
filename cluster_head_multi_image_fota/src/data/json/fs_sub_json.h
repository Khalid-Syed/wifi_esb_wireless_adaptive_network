#ifndef FS_JSON_H
#define FS_JSON_H
#include "fs_sub_types.h"

/* module to encode and decode mqtt fs JSON payloads */

/*
    CMD TOPIC
    cmd/hs-nrf5340/[mac]/fs

    CMD PAYLOAD
{
    "session_id" : 99,
    "res_topic" : "fs_res",
    "cmd" : "write",
    "file": "cfg.json"
    "data" : "hello",
}
    CMD RESPONSE TOPIC
    cmd/hs-nrf5340/[mac]/fs_resp

    CMD RESPONSE PAYLOAD
{
    "session_id" : 99,
    "status": "success"
    "data" : "hello",
}

*/

int fs_json_cmd_decode(char *json_str, fs_cmd_t *fs_cmd);
int fs_json_cmd_resp_encode(fs_cmd_resp_t *fs_cmd_resp, char *json_str, int json_str_size);
void log_fs_cmd(fs_cmd_t *fs_cmd);
void log_fs_cmd_resp(fs_cmd_resp_t *fs_cmd_resp);

#define PAYLOAD_DECODE fs_json_cmd_decode
#define RSP_ENCODE fs_json_cmd_resp_encode
#define FS_RESP_PAYLOAD_STR_SIZE_MAX 1024
#define OPTIONAL_FIELD_DEFAULT 0

#endif // FS_JSON_H
