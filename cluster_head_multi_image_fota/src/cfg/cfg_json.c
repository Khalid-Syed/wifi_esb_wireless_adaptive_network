#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "cfg.h"

#define MODULE cfg_json
LOG_MODULE_REGISTER(MODULE, CONFIG_CFG_JSON_LOG_LEVEL);

static const struct json_obj_descr gen_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct gen_cfg_t, v, JSON_TOK_STRING)
};

static const struct json_obj_descr wifi_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct wifi_cfg_t, ssid, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct wifi_cfg_t, psk, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct wifi_cfg_t, mqtt_hn, JSON_TOK_STRING)
};

static const struct json_obj_descr pwr_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct pwr_cfg_t, is_prod_unit, JSON_TOK_TRUE),
    JSON_OBJ_DESCR_PRIM(struct pwr_cfg_t, batt_mv_wifi_off, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct pwr_cfg_t, batt_mv_wifi_on, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct pwr_cfg_t, pv_mv_wifi_off, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct pwr_cfg_t, pv_mv_wifi_on, JSON_TOK_NUMBER)
};
static const struct json_obj_descr manager_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct manager_cfg_t, trd_mon_en, JSON_TOK_TRUE),
    JSON_OBJ_DESCR_PRIM(struct manager_cfg_t, wifi_to_ms, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct manager_cfg_t, mqtt_to_ms, JSON_TOK_NUMBER)};

static const struct json_obj_descr accel_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct accel_cfg_t, temp_offset, JSON_TOK_STRING)
};

static const struct json_obj_descr cfg_descr[] = {
    JSON_OBJ_DESCR_OBJECT(cfg_t, gen_cfg, gen_descr),
    JSON_OBJ_DESCR_OBJECT(cfg_t, pwr_cfg, pwr_descr),
    JSON_OBJ_DESCR_OBJECT(cfg_t, wifi_cfg, wifi_descr),
    JSON_OBJ_DESCR_OBJECT(cfg_t, manager_cfg, manager_descr),
    JSON_OBJ_DESCR_OBJECT(cfg_t, accel_cfg, accel_descr),
};

#define CFG_REQUIRED_FIELDS (0x01) // bitmask of requried fields

int cfg_decode(char *json_str, cfg_t *cfg_p)
{
    LOG_INF("%s: %s", __func__, json_str);
    if (json_str == NULL || cfg_p == NULL)
    {
        LOG_ERR("json_str or cfg* is NULL");
        return -EINVAL;
    }
    // json_obj_parse returns < 0 if error, bitmap of decoded fields on success(bit
    //  0 * is set if first field in the descriptor has been properly decoded, etc).

    int ret = json_obj_parse(json_str, strlen(json_str), cfg_descr, ARRAY_SIZE(cfg_descr), cfg_p);
    if (ret < 0)
    {
        LOG_ERR("json_obj_parse failed: err = %d", ret);
        return -ENOEXEC;
    }
    if ((ret & CFG_REQUIRED_FIELDS) != CFG_REQUIRED_FIELDS)
    {
        LOG_ERR("json_obj_parse failed: not every required field was present, field bitmask = 0x%2.2x", ret);
        return -EINVAL;
    }
    LOG_INF("json_obj_parse success, found 0x%2.2x fields", ret);
    return 0;
}
