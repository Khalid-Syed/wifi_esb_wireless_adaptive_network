#ifndef RTT_TYPES_H
#define RTT_TYPES_H

typedef struct
{
    int session_id;
    char *res_topic;
    char *operation; // "RTT"
    char *payload;   // command payload
} rtt_cmd_t;

typedef struct
{
    char *node_id;
    char *latest_ipc_msg; // latest received IPC message
} rtt_cmd_resp_t;

#endif // RTT_TYPES_H
