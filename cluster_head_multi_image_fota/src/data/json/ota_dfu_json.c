#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "ota_dfu_json.h"
#include "string_utils.h"

#define MODULE ota_dfu_json
LOG_MODULE_REGISTER(MODULE, CONFIG_OTA_DFU_JSON_LOG_LEVEL);

#define OTA_DFU_URI_STR_SIZE_MAX 128
#define OTA_DFU_FILE_STR_SIZE_MAX 64
#define OTA_DFU_INSTALL_STR_SIZE_MAX 32

char uri[OTA_DFU_URI_STR_SIZE_MAX];
char file[OTA_DFU_FILE_STR_SIZE_MAX];
char install[OTA_DFU_INSTALL_STR_SIZE_MAX];
int32_t retries;

// ota dfu command json object
static const struct json_obj_descr ota_dfu_cmd_descr[] = {
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_t, res_topic, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_t, uri, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_t, file, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_t, install, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_t, retries, JSON_TOK_NUMBER),
    // optional fields for simulation and testing
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_t, res_qos, JSON_TOK_NUMBER),
};
static const struct json_obj_descr ota_dfu_cmd_resp_descr[] = {
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_resp_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(ota_dfu_cmd_resp_t, response, JSON_TOK_STRING),
};
#define OTA_DFU_COMMAND_REQUIRED_FIELDS (0x1f) // bitmask of requried fields

// status json object
static const struct json_obj_descr ota_dfu_status_descr[] = {
    JSON_OBJ_DESCR_PRIM(ota_dfu_status_t, current_fwv, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ota_dfu_status_t, session_id, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(ota_dfu_status_t, status, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ota_dfu_status_t, progress, JSON_TOK_NUMBER),
};

int ota_dfu_json_cmd_decode(char *json_str, ota_dfu_cmd_t *ota_dfu_cmd)
{
    if (json_str == NULL || ota_dfu_cmd == NULL)
    {
        LOG_ERR("json_str or ota_dfu_cmd is NULL");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    // set optional fields to default values
    ota_dfu_cmd->res_qos = RES_QOS_DEFAULT;

    int ret = json_obj_parse(json_str, strlen(json_str), ota_dfu_cmd_descr, ARRAY_SIZE(ota_dfu_cmd_descr), ota_dfu_cmd);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed: err = %d", ret);
        return -ENOEXEC;
    }
    if ((ret & OTA_DFU_COMMAND_REQUIRED_FIELDS) != OTA_DFU_COMMAND_REQUIRED_FIELDS)
    {
        LOG_ERR("json_obj_parse failed: not every required field was present, field bitmask = 0x%2.2x", ret);
        return -EINVAL;
    }
    // copy strings to static buffers
    safe_strlcpy(uri, ota_dfu_cmd->uri, sizeof(uri));
    safe_strlcpy(file, ota_dfu_cmd->file, sizeof(file));
    safe_strlcpy(install, ota_dfu_cmd->install, sizeof(install));
    retries = ota_dfu_cmd->retries;
    // set pointers to static buffers
    ota_dfu_cmd->uri = uri;
    ota_dfu_cmd->file = file;
    ota_dfu_cmd->install = install;
    ota_dfu_cmd->retries = retries;

    return 0;
}
int ota_dfu_json_cmd_resp_encode(ota_dfu_cmd_resp_t *ota_dfu_cmd_resp, char *json_str, int json_str_size)
{
    if (ota_dfu_cmd_resp == NULL || json_str == NULL)
    {
        LOG_ERR("ota_dfu_cmd_resp or json_str is NULL");
        return -EINVAL;
    }
    int ret = json_obj_encode_buf(ota_dfu_cmd_resp_descr, ARRAY_SIZE(ota_dfu_cmd_resp_descr),
                                  ota_dfu_cmd_resp, json_str, json_str_size);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
        return ret;
    }
    return 0;
}
int ota_dfu_status_json_encode(ota_dfu_status_t *status, char *buf, int buf_len)
{

    int ret = json_obj_encode_buf(ota_dfu_status_descr, ARRAY_SIZE(ota_dfu_status_descr),
                                  status, buf, buf_len);

    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
    }
    return ret;
}
void log_ota_dfu_cmd(ota_dfu_cmd_t *ota_dfu_cmd)
{
    if (ota_dfu_cmd == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", ota_dfu_cmd->session_id);
    LOG_DBG("res_topic: %s", ota_dfu_cmd->res_topic);
    LOG_DBG("uri: %s", ota_dfu_cmd->uri);
    LOG_DBG("file: %s", ota_dfu_cmd->file);
    LOG_DBG("install: %s", ota_dfu_cmd->install);
    LOG_DBG("retries: %d", ota_dfu_cmd->retries);
}
void log_ota_dfu_cmd_resp(ota_dfu_cmd_resp_t *ota_dfu_cmd_resp)
{
    if (ota_dfu_cmd_resp == NULL)
    {
        return;
    }
    LOG_DBG("session_id: %d", ota_dfu_cmd_resp->session_id);
    LOG_DBG("reponse: %s", ota_dfu_cmd_resp->response);
}
