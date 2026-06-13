#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "node_id_json.h"

#define MODULE node_id_json
LOG_MODULE_REGISTER(MODULE, CONFIG_NODE_ID_JSON_LOG_LEVEL);

static const struct json_obj_descr node_id_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(node_id_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(node_id_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(node_id_cmd_t, operation, JSON_TOK_STRING),
};
static const struct json_obj_descr node_id_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(node_id_cmd_resp_t, node_id, JSON_TOK_STRING),
};
#define GOTO_COMMAND_REQUIRED_FIELDS (0x03) // bitmask of requried fields

int node_id_json_cmd_decode(char *json_str, node_id_cmd_t *node_id_cmd)
{
    if ((json_str == NULL) || (node_id_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    // set optional field default values
    node_id_cmd->res_topic = NODE_ID_OPERATION_DEFAULT;

    int ret = json_obj_parse(json_str, strlen(json_str), node_id_cmd_descr, ARRAY_SIZE(node_id_cmd_descr), node_id_cmd);
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
int node_id_json_cmd_resp_encode(node_id_cmd_resp_t *node_id_cmd_resp, char *json_str, int json_str_size)
{
    if ((node_id_cmd_resp == NULL) || (json_str == NULL) || (json_str_size == 0))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(node_id_cmd_resp_descr, ARRAY_SIZE(node_id_cmd_resp_descr),
                                  node_id_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_node_id_cmd(node_id_cmd_t *node_id_cmd)
{
    if (node_id_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", node_id_cmd->session_id);
    LOG_DBG("res_topic: %s", node_id_cmd->res_topic);
    LOG_DBG("operation: %s", node_id_cmd->operation);
}
void log_node_id_cmd_resp(node_id_cmd_resp_t *node_id_cmd_resp)
{
    if (node_id_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("node_id: %s", node_id_cmd_resp->node_id);
}
