#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "stow_json.h"

#define MODULE stow_json
LOG_MODULE_REGISTER(MODULE, CONFIG_STOW_JSON_LOG_LEVEL);

static const struct json_obj_descr stow_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(stow_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(stow_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(stow_cmd_t, retry, JSON_TOK_NUMBER),
};
static const struct json_obj_descr stow_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(stow_cmd_resp_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(stow_cmd_resp_t, status, JSON_TOK_STRING),
};
#define STOW_COMMAND_REQUIRED_FIELDS (0x07) // bitmask of requried fields

int stow_json_cmd_decode(char *json_str, stow_cmd_t *stow_cmd)
{
    if ((json_str == NULL) || (stow_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }

    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    int ret = json_obj_parse(json_str, strlen(json_str), stow_cmd_descr, ARRAY_SIZE(stow_cmd_descr), stow_cmd);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed: err = %d", ret);
        return -ENOEXEC;
    }
    if ((ret & STOW_COMMAND_REQUIRED_FIELDS) != STOW_COMMAND_REQUIRED_FIELDS)
    {
        LOG_ERR("json_obj_parse failed: not every required field was present");
        return -EINVAL;
    }

    return 0;
}
int stow_json_cmd_resp_encode(stow_cmd_resp_t *stow_cmd_resp, char *json_str, int json_str_size)
{
    if ((stow_cmd_resp == NULL) || (json_str == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(stow_cmd_resp_descr, ARRAY_SIZE(stow_cmd_resp_descr),
                                  stow_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_stow_cmd(stow_cmd_t *stow_cmd)
{
    if (stow_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", stow_cmd->session_id);
    LOG_DBG("res_topic: %s", stow_cmd->res_topic);
    LOG_DBG("axis_a: %d", stow_cmd->retry);
}
void log_stow_cmd_resp(stow_cmd_resp_t *stow_cmd_resp)
{
    if (stow_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", stow_cmd_resp->session_id);
    LOG_DBG("status: %s", stow_cmd_resp->status);
}
