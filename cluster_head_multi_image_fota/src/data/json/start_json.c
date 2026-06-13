#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "start_json.h"

#define MODULE start_json
LOG_MODULE_REGISTER(MODULE, CONFIG_START_JSON_LOG_LEVEL);

static const struct json_obj_descr start_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(start_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(start_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(start_cmd_t, operation, JSON_TOK_STRING),
};
static const struct json_obj_descr start_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(start_cmd_resp_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(start_cmd_resp_t, node_id, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(start_cmd_resp_t, response, JSON_TOK_STRING),
};
#define GOTO_COMMAND_REQUIRED_FIELDS (0x03) // bitmask of requried fields

int start_json_cmd_decode(char *json_str, start_cmd_t *start_cmd)
{
    if ((json_str == NULL) || (start_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    // set optional field default values
    start_cmd->operation = START_OPERATION_DEFAULT;

    int ret = json_obj_parse(json_str, strlen(json_str), start_cmd_descr, ARRAY_SIZE(start_cmd_descr), start_cmd);
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
int start_json_cmd_resp_encode(start_cmd_resp_t *start_cmd_resp, char *json_str, int json_str_size)
{
    if ((start_cmd_resp == NULL) || (json_str == NULL) || (json_str_size == 0))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(start_cmd_resp_descr, ARRAY_SIZE(start_cmd_resp_descr),
                                  start_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_start_cmd(start_cmd_t *start_cmd)
{
    if (start_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", start_cmd->session_id);
    LOG_DBG("res_topic: %s", start_cmd->res_topic);
    LOG_DBG("operation: %s", start_cmd->operation);
}
void log_start_cmd_resp(start_cmd_resp_t *start_cmd_resp)
{
    if (start_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", start_cmd_resp->session_id);
    LOG_DBG("node_id: %s", start_cmd_resp->node_id);
    LOG_DBG("response: %s", start_cmd_resp->response);
}
