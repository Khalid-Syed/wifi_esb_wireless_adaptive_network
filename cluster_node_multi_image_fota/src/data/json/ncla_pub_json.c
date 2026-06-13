#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "ncla_pub_json.h"
#include "ncla_pub_types.h"
#include "sys_pub_types.h"

#define MODULE ncla_pub_json
LOG_MODULE_REGISTER(MODULE);

static const struct json_obj_descr boot_info_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, uptime, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, boot_count, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, reboot_cause, JSON_TOK_STRING),
};

static const struct json_obj_descr ncla_pub_descr[] = {
    JSON_OBJ_DESCR_PRIM(ncla_pub_t, time_str, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ncla_pub_t, rssi, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_OBJECT(ncla_pub_t, boot_info, boot_info_descr),
    JSON_OBJ_DESCR_PRIM(ncla_pub_t, az, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ncla_pub_t, el, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ncla_pub_t, operation, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ncla_pub_t, latest_ipc_msg, JSON_TOK_STRING),
};

int ncla_pub_json_encode(ncla_pub_t *ncla_pub, char *buf, int buf_len)
{

    int ret = json_obj_encode_buf(ncla_pub_descr, ARRAY_SIZE(ncla_pub_descr),
                                  ncla_pub, buf, buf_len);

    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
    }
    return ret;
}
