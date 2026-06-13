#ifndef FS_SUB_TYPES_H
#define FS_SUB_TYPES_H

/* module to encode and decode mqtt fs JSON payloads */
typedef struct
{
    int session_id;
    char *res_topic;
    char *cmd;
    char *file;
    char *data;
} fs_cmd_t;
typedef struct
{
    int session_id;
    const char *status;
    const char *data;
} fs_cmd_resp_t;
#endif // FS_SUB_TYPES_H
