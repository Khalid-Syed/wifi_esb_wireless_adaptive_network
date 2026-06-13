#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "template_pub_json.h"

#define MODULE template_pub_json
LOG_MODULE_REGISTER(MODULE);

static const struct json_obj_descr template_pub_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct template_pub_t, foo, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct template_pub_t, bar, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct template_pub_t, state, JSON_TOK_STRING),
};

int template_pub_json_encode(template_pub_t *template_pub, char *buf, int buf_len)
{

    int ret = json_obj_encode_buf(template_pub_descr, ARRAY_SIZE(template_pub_descr),
                                  template_pub, buf, buf_len);

    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
    }
    return ret;
}
