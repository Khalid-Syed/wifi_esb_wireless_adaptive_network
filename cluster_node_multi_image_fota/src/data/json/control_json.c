#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "control_json.h"

#define MODULE control_json
LOG_MODULE_REGISTER(MODULE, CONFIG_CONTROL_JSON_LOG_LEVEL);

static const struct json_obj_descr control_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(control_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(control_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(control_cmd_t, operation, JSON_TOK_STRING),
};
static const struct json_obj_descr control_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(control_cmd_resp_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(control_cmd_resp_t, response, JSON_TOK_STRING),
};
#define GOTO_COMMAND_REQUIRED_FIELDS (0x03) // bitmask of requried fields

int control_json_cmd_decode(char *json_str, control_cmd_t *control_cmd)
{
    if ((json_str == NULL) || (control_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    // set optional field default values
    control_cmd->operation = CONTROL_OPERATION_DEFAULT;

    int ret = json_obj_parse(json_str, strlen(json_str), control_cmd_descr, ARRAY_SIZE(control_cmd_descr), control_cmd);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed: err = %d", ret);
        return -ENOEXEC;
    }
    if ((ret & GOTO_COMMAND_REQUIRED_FIELDS) != GOTO_COMMAND_REQUIRED_FIELDS)
    {
        LOG_ERR("json_obj_parse failed: not every required field was present");
        return -EINVAL;
    }

    return 0;
}
int control_json_cmd_resp_encode(control_cmd_resp_t *control_cmd_resp, char *json_str, int json_str_size)
{
    if ((control_cmd_resp == NULL) || (json_str == NULL) || (json_str_size == 0))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(control_cmd_resp_descr, ARRAY_SIZE(control_cmd_resp_descr),
                                  control_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_control_cmd(control_cmd_t *control_cmd)
{
    if (control_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", control_cmd->session_id);
    LOG_DBG("res_topic: %s", control_cmd->res_topic);
    LOG_DBG("operation: %s", control_cmd->operation);
}
void log_control_cmd_resp(control_cmd_resp_t *control_cmd_resp)
{
    if (control_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", control_cmd_resp->session_id);
    LOG_DBG("response: %s", control_cmd_resp->response);
}
