#ifndef NCLA_PUB_TYPES_H
#define NCLA_PUB_TYPES_H
#include <stddef.h>
#include "sys_pub_types.h"

#include "ipc_handler.h"

typedef struct
{
    char *time_str; // timestamp variable
    int rssi;       // wifi rssi
    boot_info_t boot_info; // node boot information
    char *az; // azimuth angle
    char *el; // elevation angle
    char *operation; // "NCLA"
    char *latest_ipc_msg; // latest received IPC message
} ncla_pub_t;

#endif // NCLA_PUB_TYPES_H