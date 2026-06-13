#ifndef CONTROL_TYPES_H
#define CONTROL_TYPES_H

typedef struct
{
    int session_id;
    char *res_topic;
    char *operation; // "CLA" / "NCLA"
} control_cmd_t;
typedef struct
{
    int session_id;
    const char *response;
} control_cmd_resp_t;

#endif // CONTROL_TYPES_H
