#ifndef WIFI_STATS_PUB_TYPES_H
#define WIFI_STATS_PUB_TYPES_H
#include <stddef.h>

typedef struct
{
    char *id_str; // node MAC id variable
    char *time_str; // timestamp variable
    int rssi;       // wifi rssi

} wifi_stats_pub_t;
#endif // WIFI_STATS_PUB_TYPES_H
