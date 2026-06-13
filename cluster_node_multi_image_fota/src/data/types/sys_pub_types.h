#ifndef SYS_PUB_TYPES_H
#define SYS_PUB_TYPES_H

typedef struct sys_info_t
{
    int uptime;
    int boot_count;
    char *reboot_cause;
    char *assert_json;
    char *fatal_error_json;
    char *con_status;
    int con_s;
} sys_info_t;

typedef struct boot_info_t
{
    int uptime;
    int boot_count;
    char *reboot_cause;
} boot_info_t;

typedef struct
{
    char *file;
    int line;
    int boot_count;
    int uptime;
} assert_t;

typedef struct
{
    char *error;
    char *reason;
    int boot_count;
    int uptime;
} fatal_error_t;

typedef struct
{
    int con_s;
    char *status;
} connection_status_t;

typedef struct sys_pub_t
{
    connection_status_t con;
    boot_info_t boot_info;
    assert_t assert;
    fatal_error_t fatal_error;
} sys_pub_t;

#endif // SYS_PUB_TYPES_H
