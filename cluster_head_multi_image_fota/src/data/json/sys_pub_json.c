#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "sys_pub_json.h"
#include "assert_post_action.h"
#include "fatal_error_handler.h"
#include "string_utils.h"

#define MODULE sys_pub_json
LOG_MODULE_REGISTER(MODULE);
static int assert_obj_decode(char *assert, assert_t *assert_obj);
static int fatal_error_obj_decode(char *fatal_err, fatal_error_t *fatal_err_obj);
static char assert_file[ASSERT_STR_LEN_MAX];
static char fatal_error_error_str[32];
static char fatal_error_reason_str[128];

static const struct json_obj_descr boot_info_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, uptime, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, boot_count, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct boot_info_t, reboot_cause, JSON_TOK_STRING),
};
static const struct json_obj_descr assert_descr[] = {
    JSON_OBJ_DESCR_PRIM(assert_t, file, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(assert_t, line, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(assert_t, boot_count, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(assert_t, uptime, JSON_TOK_NUMBER),
};
static const struct json_obj_descr fatal_error_descr[] = {
    JSON_OBJ_DESCR_PRIM(fatal_error_t, error, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(fatal_error_t, reason, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(fatal_error_t, boot_count, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(fatal_error_t, uptime, JSON_TOK_NUMBER),
};
static const struct json_obj_descr connection_descr[] = {
    JSON_OBJ_DESCR_PRIM(connection_status_t, con_s, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(connection_status_t, status, JSON_TOK_STRING),
};
static const struct json_obj_descr sys_pub_descr[] = {
    JSON_OBJ_DESCR_OBJECT(sys_pub_t, con, connection_descr),
    JSON_OBJ_DESCR_OBJECT(sys_pub_t, boot_info, boot_info_descr),
    JSON_OBJ_DESCR_OBJECT(sys_pub_t, assert, assert_descr),
    JSON_OBJ_DESCR_OBJECT(sys_pub_t, fatal_error, fatal_error_descr),
};

int sys_pub_json_encode(sys_info_t *sys_info, char *buf, int buf_len)
{
    sys_pub_t sys_pub;

    // copy the uptime, boot_count, and reboot_cause into a boot_info object
    sys_pub.boot_info.uptime = sys_info->uptime;
    sys_pub.boot_info.boot_count = sys_info->boot_count;
    sys_pub.boot_info.reboot_cause = sys_info->reboot_cause;

    // set connection status string and connection uptime seconds
    sys_pub.con.con_s = sys_info->con_s;
    sys_pub.con.status = sys_info->con_status;

    // the assert string in the sys_info struct is already in JSON, so decode it into an assert_t object
    LOG_DBG("assert_json = \"%s\"", sys_info->assert_json);
    int ret = assert_obj_decode(sys_info->assert_json, &sys_pub.assert);
    if (ret < 0)
    {
        LOG_ERR("assert_obj_decode failed ret %d obj=\"%s\" ", ret, sys_info->assert_json);
    }

    // the fatal error string in the sys_info struct is already in JSON, so decode it into an fatal_error_t object
    ret = fatal_error_obj_decode(sys_info->fatal_error_json, &sys_pub.fatal_error);
    if (ret < 0)
    {
        LOG_ERR("fatal_error_obj_decode failed ret %d obj=\"%s\"", ret, sys_info->fatal_error_json);
    }

    ret = json_obj_encode_buf(sys_pub_descr, ARRAY_SIZE(sys_pub_descr), &sys_pub, buf, buf_len);
    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
    }

    return ret;
}

static int assert_obj_decode(char *assert, assert_t *assert_obj)
{
    // assert is stored as a json string hand built like this
    // sprintf(assert_msg, "{\"file\":\"%s\","
    //                 "\"line\":%d,"
    //                 "\"boot_count\":%d,"
    //                 "\"uptime\":%lld"
    //                 "}",

    // need to make a temp copy because json_obj_parse is destructive and modifies the string
    char tmp[ASSERT_STR_LEN_MAX];
    safe_strlcpy(tmp, assert, sizeof(tmp));
    int ret = json_obj_parse(tmp, strlen(tmp), assert_descr, ARRAY_SIZE(assert_descr), assert_obj);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed ret %d len = %d", ret, strlen(assert));
        LOG_ERR("assert %s", assert);
        assert_obj->file = "";
        assert_obj->line = 0;
        assert_obj->boot_count = 0;
        assert_obj->uptime = 0;
    }
    safe_strlcpy(assert_file, assert_obj->file, sizeof(assert_file));
    assert_obj->file = assert_file;
    return ret;
}

static int fatal_error_obj_decode(char *fatal_err, fatal_error_t *fatal_err_obj)
{
    // fatal errors are stored as a json string hand built like this

    // sprintf(retained.fatal_err_str, "{\"error\":\"fatal\","
    //                                 "\"reason\":\"%s\","
    //                                 "\"boot_count\":%d,"
    //                                 "\"uptime\":%lld"
    //                                 "}",

    // need to make a temp copy because json_obj_parse is destructive and modifies the string
    char tmp[FATAL_ERROR_STR_LEN_MAX];
    safe_strlcpy(tmp, fatal_err, sizeof(tmp));
    int ret = json_obj_parse(tmp, strlen(tmp), fatal_error_descr, ARRAY_SIZE(fatal_error_descr), fatal_err_obj);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed ret %d len = %d", ret, strlen(fatal_err));
        LOG_ERR("fatal_err %s", fatal_err);
        fatal_err_obj->error = "";
        fatal_err_obj->reason = "";
        fatal_err_obj->boot_count = 0;
        fatal_err_obj->uptime = 0;
    }
    safe_strlcpy(fatal_error_error_str, fatal_err_obj->error, sizeof(fatal_error_error_str));
    safe_strlcpy(fatal_error_reason_str, fatal_err_obj->reason, sizeof(fatal_error_reason_str));
    fatal_err_obj->error = fatal_error_error_str;
    fatal_err_obj->reason = fatal_error_reason_str;
    return ret;
}
