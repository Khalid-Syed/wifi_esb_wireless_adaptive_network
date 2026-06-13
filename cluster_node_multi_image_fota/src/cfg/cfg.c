#include <zephyr/kernel.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "storage.h"
#include "cfg.h"
#include "cfg_defaults.h"
#include "cfg_json.h"
#include "string_utils.h"
#include "manager_connectivity_monitor.h"

#define MODULE cfg
LOG_MODULE_REGISTER(MODULE, CONFIG_CFG_LOG_LEVEL);

static int cfg_file_get(char *cfg_file_name, int len);
static int cfg_file_read(char *cfg_file_name, char *json_str, int json_str_len);

// the config struct is defined in cfg.h
cfg_t cfg;

float accel_temp_offset_flt;

// char* members of the struct need to memory allocated to
// be persistant and not live just on the stack
char v[CFG_VERSION_LEN_MAX];
char ssid[CFG_SSID_LEN_MAX];
char psk[CFG_PSK_LEN_MAX];
char mqtt_hn[CFG_MQTT_HN_LEN_MAX];
char temp_offset[CFG_ACCEL_TEMP_OFFSET_LEN_MAX];

int cfg_init(void)
{

    char cfg_file_name[MAX_PATH_LEN];
    char json_str[1024];
    int rc;
    int ret = 0;

    LOG_DBG("load defaults into cfg struct in case config file is not found");
    cfg_defaults(cfg_get());

    //get name of config file from storage
    cfg_file_get(cfg_file_name, sizeof(cfg_file_name));

    // read actual config file
    rc = cfg_file_read(cfg_file_name, json_str, sizeof(json_str));
    if (rc < 0)
    {
        LOG_ERR("No config file \"%s\" found, using defaults", cfg_file_name);
    }
    else
    {
        // decode json into cfg struct
        cfg_decode(json_str, cfg_get());
    }

    // the decode will make copies of ints and bools,
    // but char* members of struct need to be copied into global variables
    // as the string data is stored in the json_str buffer (on the stack) and will be lost later
    safe_strlcpy(v, cfg_get()->gen_cfg.v, CFG_VERSION_LEN_MAX);
    safe_strlcpy(ssid, cfg_get()->wifi_cfg.ssid, CFG_SSID_LEN_MAX);
    safe_strlcpy(psk, cfg_get()->wifi_cfg.psk, CFG_PSK_LEN_MAX);
    safe_strlcpy(mqtt_hn, cfg_get()->wifi_cfg.mqtt_hn, CFG_MQTT_HN_LEN_MAX);
    safe_strlcpy(temp_offset, cfg_get()->accel_cfg.temp_offset, CFG_ACCEL_TEMP_OFFSET_LEN_MAX);

    // now point the struct members to the global variables
    cfg.gen_cfg.v = v;
    cfg.wifi_cfg.ssid = ssid;
    cfg.wifi_cfg.psk = psk;
    cfg.wifi_cfg.mqtt_hn = mqtt_hn;

    cfg.accel_cfg.temp_offset = temp_offset;

    ret = sscanf(cfg.accel_cfg.temp_offset, "%f", &accel_temp_offset_flt);

    LOG_ERR("Before conversion: %s", cfg.accel_cfg.temp_offset);
    LOG_ERR("After conversion: %f", accel_temp_offset_flt);

     // If not all values were converted, use the default configs
    if (ret != 1)
    {
        LOG_ERR("Can't convert accel temp offset to float, using default value");
        accel_temp_offset_flt = 25.0;
    }

    // Sadman's note: setting the following variables in cfg_init() instead of reading from cfg.json file in the node
    cfg.manager_cfg.wifi_to_ms = MANAGER_CONNECTIVITY_MONITOR_WIFI_TIMEOUT_MS;
    cfg.manager_cfg.mqtt_to_ms = MANAGER_CONNECTIVITY_MONITOR_TRANSPORT_TIMEOUT_MS;

    // Sadman's node: setting a static SSID and MQTT Broker IP for testing [DELETE after testing]
    cfg.wifi_cfg.ssid = "G5-Heliostat-AP-5GHz_4";
    cfg.wifi_cfg.mqtt_hn = "10.30.224.5";

    log_cfg(cfg_get());

    return 0;
}

cfg_t *cfg_get(void)
{
    return &cfg;
}
char *cfg_get_version(void)
{
    return cfg.gen_cfg.v;
}
char *cfg_get_wifi_ssid(void)
{
    return cfg.wifi_cfg.ssid;
}
char *cfg_get_wifi_psk(void)
{
    return cfg.wifi_cfg.psk;
}
char *cfg_get_mqtt_hostname(void)
{
    return cfg.wifi_cfg.mqtt_hn;
}

float cfg_get_accel_temp_offset(void)
{
    return accel_temp_offset_flt;
}

int32_t cfg_get_batt_mv_wifi_off(void)
{
    return cfg.pwr_cfg.batt_mv_wifi_off;
}

int32_t cfg_get_batt_mv_wifi_on(void)
{
    return cfg.pwr_cfg.batt_mv_wifi_on;
}

int32_t cfg_get_pv_mv_wifi_off(void)
{
    return cfg.pwr_cfg.pv_mv_wifi_off;
}

int32_t cfg_get_pv_mv_wifi_on(void)
{
    return cfg.pwr_cfg.pv_mv_wifi_on;
}

void log_cfg(cfg_t *cfg_p)
{
    if (cfg_p == NULL)
    {
        LOG_ERR("cfg_p is NULL");
        return;
    }
    LOG_INF("v: %s", cfg_p->gen_cfg.v);
    LOG_INF("ssid: %s", cfg_p->wifi_cfg.ssid);
    LOG_INF("psk: %s", cfg_p->wifi_cfg.psk);
    LOG_INF("mqtt_hn: %s", cfg_p->wifi_cfg.mqtt_hn);

    LOG_INF("pwr config:");
    LOG_INF("is_prod_unit: %d", cfg_p->pwr_cfg.is_prod_unit);
    LOG_INF("batt_mv_wifi_off: %d", cfg_p->pwr_cfg.batt_mv_wifi_off);
    LOG_INF("batt_mv_wifi_on: %d", cfg_p->pwr_cfg.batt_mv_wifi_on);
    LOG_INF("pv_mv_wifi_off: %d", cfg_p->pwr_cfg.pv_mv_wifi_off);
    LOG_INF("pv_mv_wifi_on: %d", cfg_p->pwr_cfg.pv_mv_wifi_on);

    LOG_INF("manager:");
    LOG_INF("trd_mon_en: %d", cfg_p->manager_cfg.trd_mon_en);
    LOG_INF("wifi_to_ms: %d", cfg_p->manager_cfg.wifi_to_ms);
    LOG_INF("mqtt_to_ms: %d", cfg_p->manager_cfg.mqtt_to_ms);

    LOG_INF("accel: ");
    LOG_INF("temp_offset: %s", cfg_p->accel_cfg.temp_offset);
}

/** PRIVATE FUNCS **/
static int cfg_file_get(char *cfg_file_name, int len)
{
    struct fs_file_t file;
    char path[MAX_PATH_LEN];
    int rc;

    struct fs_mount_t *mp = storage_mount_point_get();

    // open file to read name of actual config file
    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, CONFIG_HELIOGEN_CONFIG_FILE_CONFIG);
    LOG_DBG("%s opening %s", __func__, path);

    fs_file_t_init(&file);
    rc = fs_open(&file, path, FS_O_RDWR);
    if (rc < 0)
    {
        LOG_ERR("FAIL: open %s: %d, use default path %s as cfg file", path, rc, CONFIG_HELIOGEN_CONFIG_FILE_DEFAULT);
        safe_strlcpy(cfg_file_name, CONFIG_HELIOGEN_CONFIG_FILE_DEFAULT, len);
        goto fini;
    }
    rc = fs_read(&file, cfg_file_name, len);
    if (rc < 0)
    {
        LOG_ERR("FAIL: read %s: [rd:%d], use default path %s as cfg file", path, rc, CONFIG_HELIOGEN_CONFIG_FILE_DEFAULT);
        safe_strlcpy(cfg_file_name, CONFIG_HELIOGEN_CONFIG_FILE_DEFAULT, len);
        goto fini;
    }
    cfg_file_name[rc] = 0; // null terminate JIC
fini:
    fs_close(&file); // try to close no matter what
    LOG_DBG("%s cfg_file_name: %s", __func__, cfg_file_name);
    return rc;
}

static int cfg_file_read(char *cfg_file_name, char *json_str, int json_str_len)
{
    struct fs_file_t file;
    char path[MAX_PATH_LEN + 1];
    int rc;

    struct fs_mount_t *mp = storage_mount_point_get();

    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, cfg_file_name);
    fs_file_t_init(&file);
    rc = fs_open(&file, path, FS_O_READ);
    if (rc < 0)
    {
        LOG_ERR("FAIL: open %s: %d, using cfg defaults", path, rc);
        goto fini;
    }
    rc = fs_read(&file, json_str, json_str_len);
    if (rc < 0)
    {
        LOG_ERR("FAIL: read %s: [rd:%d], using cfg defaults", path, rc);
        goto fini;
    }

    json_str[rc] = 0; // null terminate JIK
    LOG_DBG("json_str: %s", json_str);
fini:
    fs_close(&file);
    return rc;
}