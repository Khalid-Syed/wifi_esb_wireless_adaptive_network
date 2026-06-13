#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "rtt_json.h"

#define MODULE rtt_json
LOG_MODULE_REGISTER(MODULE, CONFIG_RTT_JSON_LOG_LEVEL);

static const struct json_obj_descr rtt_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(rtt_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(rtt_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(rtt_cmd_t, operation, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(rtt_cmd_t, payload, JSON_TOK_STRING),
};
static const struct json_obj_descr rtt_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(rtt_cmd_resp_t, node_id, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(rtt_cmd_resp_t, latest_ipc_msg, JSON_TOK_STRING),
};
#define GOTO_COMMAND_REQUIRED_FIELDS (0x03) // bitmask of requried fields

int rtt_json_cmd_decode(char *json_str, rtt_cmd_t *rtt_cmd)
{
    if ((json_str == NULL) || (rtt_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    // set optional field default values
    rtt_cmd->res_topic = RTT_RES_TOPIC_DEFAULT;

    int ret = json_obj_parse(json_str, strlen(json_str), rtt_cmd_descr, ARRAY_SIZE(rtt_cmd_descr), rtt_cmd);
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
int rtt_json_cmd_resp_encode(rtt_cmd_resp_t *rtt_cmd_resp, char *json_str, int json_str_size)
{
    if ((rtt_cmd_resp == NULL) || (json_str == NULL) || (json_str_size == 0))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(rtt_cmd_resp_descr, ARRAY_SIZE(rtt_cmd_resp_descr),
                                  rtt_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_rtt_cmd(rtt_cmd_t *rtt_cmd)
{
    if (rtt_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", rtt_cmd->session_id);
    LOG_DBG("res_topic: %s", rtt_cmd->res_topic);
    LOG_DBG("operation: %s", rtt_cmd->operation);
    LOG_DBG("payload: %s", rtt_cmd->payload);
}
void log_rtt_cmd_resp(rtt_cmd_resp_t *rtt_cmd_resp)
{
    if (rtt_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("node_id: %s", rtt_cmd_resp->node_id);
    LOG_DBG("latest_ipc_msg: %s", rtt_cmd_resp->latest_ipc_msg);
}
