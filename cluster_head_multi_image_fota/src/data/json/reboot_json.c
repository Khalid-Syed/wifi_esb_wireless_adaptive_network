#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "reboot_json.h"

#define MODULE reboot_json
LOG_MODULE_REGISTER(MODULE, CONFIG_REBOOT_JSON_LOG_LEVEL);

static const struct json_obj_descr reboot_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(reboot_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(reboot_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(reboot_cmd_t, delay, JSON_TOK_NUMBER),
};
static const struct json_obj_descr reboot_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(reboot_cmd_resp_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(reboot_cmd_resp_t, response, JSON_TOK_STRING),
};
#define GOTO_COMMAND_REQUIRED_FIELDS (0x03) // bitmask of requried fields

int reboot_json_cmd_decode(char *json_str, reboot_cmd_t *reboot_cmd)
{
    if ((json_str == NULL) || (reboot_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    // set optional field default values
    reboot_cmd->delay = REBOOT_DELAY_DEFAULT;

    int ret = json_obj_parse(json_str, strlen(json_str), reboot_cmd_descr, ARRAY_SIZE(reboot_cmd_descr), reboot_cmd);
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
int reboot_json_cmd_resp_encode(reboot_cmd_resp_t *reboot_cmd_resp, char *json_str, int json_str_size)
{
    if ((reboot_cmd_resp == NULL) || (json_str == NULL) || (json_str_size == 0))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(reboot_cmd_resp_descr, ARRAY_SIZE(reboot_cmd_resp_descr),
                                  reboot_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_reboot_cmd(reboot_cmd_t *reboot_cmd)
{
    if (reboot_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", reboot_cmd->session_id);
    LOG_DBG("res_topic: %s", reboot_cmd->res_topic);
    LOG_DBG("delay: %d", reboot_cmd->delay);
}
void log_reboot_cmd_resp(reboot_cmd_resp_t *reboot_cmd_resp)
{
    if (reboot_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", reboot_cmd_resp->session_id);
    LOG_DBG("response: %s", reboot_cmd_resp->response);
}
