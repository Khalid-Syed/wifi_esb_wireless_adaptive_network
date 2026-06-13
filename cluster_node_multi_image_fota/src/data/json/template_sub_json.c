#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "template_sub_json.h"

#define MODULE template_json
LOG_MODULE_REGISTER(MODULE, CONFIG_TEMPLATE_SUB_JSON_LOG_LEVEL);

static const struct json_obj_descr template_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(template_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(template_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(template_cmd_t, data, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(template_cmd_t, optional_field, JSON_TOK_NUMBER),
};
static const struct json_obj_descr template_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(template_cmd_resp_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(template_cmd_resp_t, status, JSON_TOK_STRING),
};
#define TEMPLATE_COMMAND_REQUIRED_FIELDS (0x07) // bitmask of requried fields

int template_json_cmd_decode(char *json_str, template_cmd_t *template_cmd)
{
    if ((json_str == NULL) || (template_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    // init optional fields
    template_cmd->optional_field = OPTIONAL_FIELD_DEFAULT;

    int ret = json_obj_parse(json_str, strlen(json_str), template_cmd_descr, ARRAY_SIZE(template_cmd_descr), template_cmd);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed: err = %d", ret);
        return -ENOEXEC;
    }
    if ((ret & TEMPLATE_COMMAND_REQUIRED_FIELDS) != TEMPLATE_COMMAND_REQUIRED_FIELDS)
    {
        LOG_ERR("json_obj_parse failed: not every required field was present");
        return -EINVAL;
    }

    return 0;
}
int template_json_cmd_resp_encode(template_cmd_resp_t *template_cmd_resp, char *json_str, int json_str_size)
{
    if ((template_cmd_resp == NULL) || (json_str == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(template_cmd_resp_descr, ARRAY_SIZE(template_cmd_resp_descr),
                                  template_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_template_cmd(template_cmd_t *template_cmd)
{
    if (template_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", template_cmd->session_id);
    LOG_DBG("res_topic: %s", template_cmd->res_topic);
    LOG_DBG("data: %d", template_cmd->data);
}
void log_template_cmd_resp(template_cmd_resp_t *template_cmd_resp)
{
    if (template_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", template_cmd_resp->session_id);
    LOG_DBG("status: %s", template_cmd_resp->status);
}
