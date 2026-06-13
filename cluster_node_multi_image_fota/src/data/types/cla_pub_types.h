#ifndef CLA_PUB_TYPES_H
#define CLA_PUB_TYPES_H
#include <stddef.h>
#include "sys_pub_types.h"

typedef struct
{
    char *time_str; // timestamp variable
    int rssi;       // wifi rssi
    boot_info_t boot_info; // node boot information
    char *az; // azimuth angle
    char *el; // elevation angle
    char *operation; // "CLA"
} cla_pub_t;

#endif // CLA_PUB_TYPES_H