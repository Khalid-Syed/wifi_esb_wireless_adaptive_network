#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "remote_pub_sub_json.h"

#define MODULE remote_pub_json
LOG_MODULE_REGISTER(MODULE, CONFIG_REMOTE_PUB_JSON_LOG_LEVEL);

static const struct json_obj_descr remote_pub_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(remote_pub_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(remote_pub_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(remote_pub_cmd_t, pub_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(remote_pub_cmd_t, delay_ms, JSON_TOK_NUMBER),
};
static const struct json_obj_descr remote_pub_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(remote_pub_cmd_resp_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(remote_pub_cmd_resp_t, status, JSON_TOK_STRING),
};
#define REMOTE_PUB_COMMAND_REQUIRED_FIELDS (0x07) // bitmask of requried fields

int remote_pub_json_cmd_decode(char *json_str, remote_pub_cmd_t *remote_pub_cmd)
{
    if ((json_str == NULL) || (remote_pub_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    int ret = json_obj_parse(json_str, strlen(json_str), remote_pub_cmd_descr, ARRAY_SIZE(remote_pub_cmd_descr), remote_pub_cmd);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed: err = %d", ret);
        return -ENOEXEC;
    }
    if ((ret & REMOTE_PUB_COMMAND_REQUIRED_FIELDS) != REMOTE_PUB_COMMAND_REQUIRED_FIELDS)
    {
        LOG_ERR("json_obj_parse failed: not every required field was present");
        return -EINVAL;
    }

    return 0;
}
int remote_pub_json_cmd_resp_encode(remote_pub_cmd_resp_t *remote_pub_cmd_resp, char *json_str, int json_str_size)
{
    if ((remote_pub_cmd_resp == NULL) || (json_str == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(remote_pub_cmd_resp_descr, ARRAY_SIZE(remote_pub_cmd_resp_descr),
                                  remote_pub_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_remote_pub_cmd(remote_pub_cmd_t *remote_pub_cmd)
{
    if (remote_pub_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", remote_pub_cmd->session_id);
    LOG_DBG("res_topic: %s", remote_pub_cmd->res_topic);
    LOG_DBG("pub_topic: %s", remote_pub_cmd->pub_topic);
    LOG_DBG("delay_ms: %d", remote_pub_cmd->delay_ms);
}
void log_remote_pub_cmd_resp(remote_pub_cmd_resp_t *remote_pub_cmd_resp)
{
    if (remote_pub_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", remote_pub_cmd_resp->session_id);
    LOG_DBG("status: %s", remote_pub_cmd_resp->status);
}
