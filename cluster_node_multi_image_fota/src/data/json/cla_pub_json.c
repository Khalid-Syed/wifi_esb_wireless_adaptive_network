#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "cla_pub_json.h"
#include "cla_pub_types.h"
#include "sys_pub_types.h"

#define MODULE cla_pub_json
LOG_MODULE_REGISTER(MODULE);

static const struct json_obj_descr boot_info_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, uptime, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, boot_count, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, reboot_cause, JSON_TOK_STRING),
};

static const struct json_obj_descr cla_pub_descr[] = {
    JSON_OBJ_DESCR_PRIM(cla_pub_t, time_str, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(cla_pub_t, rssi, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_OBJECT(cla_pub_t, boot_info, boot_info_descr),
    JSON_OBJ_DESCR_PRIM(cla_pub_t, az, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(cla_pub_t, el, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(cla_pub_t, operation, JSON_TOK_STRING),
};

int cla_pub_json_encode(cla_pub_t *cla_pub, char *buf, int buf_len)
{

    int ret = json_obj_encode_buf(cla_pub_descr, ARRAY_SIZE(cla_pub_descr),
                                  cla_pub, buf, buf_len);

    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
    }
    return ret;
}
