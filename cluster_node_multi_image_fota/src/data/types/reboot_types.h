#ifndef REBOOT_TYPES_H
#define REBOOT_TYPES_H

typedef struct
{
    int session_id;
    char *res_topic;
    int delay;
} reboot_cmd_t;
typedef struct
{
    int session_id;
    const char *response;
} reboot_cmd_resp_t;

#endif // REBOOT_TYPES_H
