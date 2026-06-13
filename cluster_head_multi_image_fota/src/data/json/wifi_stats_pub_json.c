#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "wifi_stats_pub_json.h"
#include "wifi_stats_pub_types.h"

#define MODULE wifi_stats_pub_json
LOG_MODULE_REGISTER(MODULE);

static const struct json_obj_descr wifi_stats_pub_descr[] = {
    JSON_OBJ_DESCR_PRIM(wifi_stats_pub_t, id_str, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(wifi_stats_pub_t, time_str, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(wifi_stats_pub_t, rssi, JSON_TOK_NUMBER),
};

int wifi_stats_pub_json_encode(wifi_stats_pub_t *wifi_stats_pub, char *buf, int buf_len)
{

    int ret = json_obj_encode_buf(wifi_stats_pub_descr, ARRAY_SIZE(wifi_stats_pub_descr),
                                  wifi_stats_pub, buf, buf_len);

    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
    }
    return ret;
}
