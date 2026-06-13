#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "mppt_pub_json.h"

#define MODULE mppt_pub_json
LOG_MODULE_REGISTER(MODULE, CONFIG_MPPT_PUB_JSON_LOG_LEVEL);

static const struct json_obj_descr mppt_descr[] = {
    JSON_OBJ_DESCR_PRIM(mppt_pub_t, mppt_state, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(mppt_pub_t, gate_duty, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(mppt_pub_t, mv, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(mppt_pub_t, ma, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(mppt_pub_t, mw, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(mppt_pub_t, bat_mv, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(mppt_pub_t, tenth_C, JSON_TOK_NUMBER),
};

int mppt_pub_json_encode(mppt_pub_t *mppt_pub, char *buf, int buf_len)
{
    int ret = json_obj_encode_buf(mppt_descr, ARRAY_SIZE(mppt_descr),
                                  mppt_pub, buf, buf_len);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
    }
    return ret;
}
