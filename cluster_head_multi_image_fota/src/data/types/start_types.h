#ifndef START_TYPES_H
#define START_TYPES_H

typedef struct
{
    int session_id;
    char *res_topic;
    char *operation; // "START"
} start_cmd_t;
typedef struct
{
    int session_id;
    char *node_id;
    const char *response;
} start_cmd_resp_t;

#endif // START_TYPES_H
