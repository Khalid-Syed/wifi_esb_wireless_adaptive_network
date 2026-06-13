#ifndef REMOTE_PUB_SUB_TYPES_H
#define REMOTE_PUB_SUB_TYPES_H

/* module to encode and decode mqtt remote_pub JSON payloads */
typedef struct
{
    int session_id;
    char *res_topic;
    char *pub_topic;
    int delay_ms;
} remote_pub_cmd_t;
typedef struct
{
    int session_id;
    const char *status;
} remote_pub_cmd_resp_t;
#endif // REMOTE_PUB_SUB_TYPES_H
