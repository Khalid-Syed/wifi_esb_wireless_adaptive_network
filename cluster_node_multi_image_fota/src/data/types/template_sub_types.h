#ifndef TEMPLATE_SUB_TYPES_H
#define TEMPLATE_SUB_TYPES_H

/* module to encode and decode mqtt template JSON payloads */
typedef struct
{
    int session_id;
    char *res_topic;
    int data;
    // optional field
    int optional_field;
} template_cmd_t;
typedef struct
{
    int session_id;
    const char *status;
} template_cmd_resp_t;
#endif // TEMPLATE_SUB_TYPES_H
