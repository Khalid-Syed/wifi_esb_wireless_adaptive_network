#ifndef NODE_ID_TYPES_H
#define NODE_ID_TYPES_H

typedef struct
{
    int session_id;
    char *res_topic;
    char *operation; // "GET_NODE_ID"
} node_id_cmd_t;

typedef struct
{
    char *node_id;
} node_id_cmd_resp_t;

#endif // NODE_ID_TYPES_H
