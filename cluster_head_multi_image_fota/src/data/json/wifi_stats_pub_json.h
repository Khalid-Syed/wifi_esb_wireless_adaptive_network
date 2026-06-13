#ifndef WIFI_STATS_PUB_JSON_H
#define WIFI_STATS_PUB_JSON_H
#include "wifi_stats_pub_types.h"

#define WIFI_STATS_PUB_PAYLOAD_STR_SIZE_MAX 1024
#define PAYLOAD_ENCODE wifi_stats_pub_json_encode
int wifi_stats_pub_json_encode(wifi_stats_pub_t *wifi_stats_pub, char *buf, int buf_len);

#endif // WIFI_STATS_PUB_JSON_H
