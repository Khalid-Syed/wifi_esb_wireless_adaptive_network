#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "fs_sub_json.h"

#define MODULE fs_json
LOG_MODULE_REGISTER(MODULE, CONFIG_FS_SUB_JSON_LOG_LEVEL);

static const struct json_obj_descr fs_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(fs_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(fs_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(fs_cmd_t, cmd, JSON_TOK_STRING),  // read or write or ls
    JSON_OBJ_DESCR_PRIM(fs_cmd_t, file, JSON_TOK_STRING), // file name (empty string if command == ls)
    JSON_OBJ_DESCR_PRIM(fs_cmd_t, data, JSON_TOK_STRING), // fs data - may be string in JSON (empty string if command == read)
};
static const struct json_obj_descr fs_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(fs_cmd_resp_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(fs_cmd_resp_t, status, JSON_TOK_STRING), // ok or error if fs json is invalid
    JSON_OBJ_DESCR_PRIM(fs_cmd_resp_t, data, JSON_TOK_STRING),   // fs string in JSON if status == ok
};
#define FS_COMMAND_REQUIRED_FIELDS (0x1F) // bitmask of requried fields
/* removed escaped quotes from string */
static void remove_escaped_escapes(char *str)
{
    char *src = str, *dst = str;

    while (*src)
    {
        if (*src == '\\')
        {
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/**
 * @brief decode JSON string to fs_cmd_t structure
 *
 * @param json_str
 * @param fs_cmd
 * @return int
 */
int fs_json_cmd_decode(char *json_str, fs_cmd_t *fs_cmd)
{
    if ((json_str == NULL) || (fs_cmd == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).
    LOG_DBG("json_str: %s", json_str);
    int ret = json_obj_parse(json_str, strlen(json_str), fs_cmd_descr, ARRAY_SIZE(fs_cmd_descr), fs_cmd);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed: err = %d", ret);
        return -ENOEXEC;
    }
    if ((ret & FS_COMMAND_REQUIRED_FIELDS) != FS_COMMAND_REQUIRED_FIELDS)
    {
        LOG_ERR("json_obj_parse failed: not every required field was present");
        return -EINVAL;
    }
    // the string may have escaped qoutes, remove the escapes
    // the json_obj_parse func does not remove them
    remove_escaped_escapes(fs_cmd->data);

    return 0;
}
int fs_json_cmd_resp_encode(fs_cmd_resp_t *fs_cmd_resp, char *json_str, int json_str_size)
{
    if ((fs_cmd_resp == NULL) || (json_str == NULL))
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    LOG_DBG("json_str_size: %d", json_str_size);
    log_fs_cmd_resp(fs_cmd_resp);
    int ret = json_obj_encode_buf(fs_cmd_resp_descr, ARRAY_SIZE(fs_cmd_resp_descr),
                                  fs_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
void log_fs_cmd(fs_cmd_t *fs_cmd)
{
    if (fs_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", fs_cmd->session_id);
    LOG_DBG("res_topic: %s", fs_cmd->res_topic);
    LOG_DBG("cmd: %s", fs_cmd->cmd);
    LOG_DBG("file: %s", fs_cmd->file);
    LOG_DBG("data: %s", fs_cmd->data);
}
void log_fs_cmd_resp(fs_cmd_resp_t *fs_cmd_resp)
{
    if (fs_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", fs_cmd_resp->session_id);
    LOG_DBG("status: %s", fs_cmd_resp->status);
    LOG_DBG("data: %s", fs_cmd_resp->data);
}
