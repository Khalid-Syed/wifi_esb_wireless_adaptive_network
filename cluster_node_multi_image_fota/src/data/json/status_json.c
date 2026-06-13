#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "status_json.h"
#include "status_obj.h"
#include "motor_constants.h"

#define MODULE status_json
LOG_MODULE_REGISTER(MODULE_CONFIG_STATUS_JSON_LOG_LEVEL);

static const struct json_obj_descr axis_pos_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct axis_pos, step_pos, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct axis_pos, tar_pos, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct axis_pos, last_skip, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct axis_pos, tot_skip, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct axis_pos, state, JSON_TOK_STRING),
};
static const struct json_obj_descr axis_cfg_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct axis_cfg, duty, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct axis_cfg, per, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct axis_cfg, i_lim, JSON_TOK_NUMBER),
};
static const struct json_obj_descr axis_telem_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct axis_telem, b_emf, JSON_TOK_NUMBER),
};
static const struct json_obj_descr accel_telem_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct accel_telem, p, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct accel_telem, r, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct accel_telem, temp_c, JSON_TOK_STRING),
};
static const struct json_obj_descr pos_descr[] = {
    JSON_OBJ_DESCR_OBJECT(struct pos, axis_0, axis_pos_descr),
    JSON_OBJ_DESCR_OBJECT(struct pos, axis_1, axis_pos_descr),
    JSON_OBJ_DESCR_PRIM(struct pos, ts, JSON_TOK_NUMBER),
};
static const struct json_obj_descr cfg_descr[] = {
    JSON_OBJ_DESCR_OBJECT(struct cfg, axis_0, axis_cfg_descr),
    JSON_OBJ_DESCR_OBJECT(struct cfg, axis_1, axis_cfg_descr),
};
static const struct json_obj_descr telem_descr[] = {
    JSON_OBJ_DESCR_OBJECT(struct telem, axis_0, axis_telem_descr),
    JSON_OBJ_DESCR_OBJECT(struct telem, axis_1, axis_telem_descr),
    JSON_OBJ_DESCR_OBJECT(struct telem, accel, accel_telem_descr),
};
static const struct json_obj_descr motor_fsm_state_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct motor_fsm_state, state, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct motor_fsm_state, h_state, JSON_TOK_STRING),
};
static const struct json_obj_descr status_descr[] = {
    JSON_OBJ_DESCR_OBJECT(status_t, pos, pos_descr),
    JSON_OBJ_DESCR_OBJECT(status_t, cfg, cfg_descr),
    JSON_OBJ_DESCR_OBJECT(status_t, telem, telem_descr),
    JSON_OBJ_DESCR_OBJECT(status_t, motor_fsm_state, motor_fsm_state_descr),
};

int status_json_encode(status_t *status, char *buf, int buf_len)
{

    int ret = json_obj_encode_buf(status_descr, ARRAY_SIZE(status_descr),
                                  status, buf, buf_len);

    if (ret != 0)
    {
        LOG_ERR("json_obj_encode_alloc failed ret %d", ret);
    }
    return ret;
}
