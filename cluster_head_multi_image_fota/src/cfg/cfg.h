#ifndef __CFG_H__
#define __CFG_H__
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define CFG_VERSION_LEN_MAX 64
#define CFG_SSID_LEN_MAX 64
#define CFG_PSK_LEN_MAX 64
#define CFG_MQTT_HN_LEN_MAX 64
#define CFG_THREAD_MONITOR_ENABLE_LEN_MAX 64
#define CFG_PID_LEN_MAX 16
#define CFG_ACCEL_TEMP_OFFSET_LEN_MAX 16

typedef struct
{
    float k_p;
    float k_i;
    float k_d;
} pid_flt_cfg_t;

struct pid_cfg_t
{
    char *k_p;
    char *k_i;
    char *k_d;
};

struct gen_cfg_t
{
    char *v;
};

struct wifi_cfg_t
{
    char *ssid;
    char *psk;
    char *mqtt_hn;
};

struct pwr_cfg_t
{
    bool is_prod_unit;
    int batt_mv_wifi_off;
    int batt_mv_wifi_on;
    int pv_mv_wifi_off;
    int pv_mv_wifi_on;
};
struct manager_cfg_t
{
    bool trd_mon_en;
    int wifi_to_ms;
    int mqtt_to_ms;
};

struct accel_cfg_t
{
    char *temp_offset;
};

typedef struct
{
    struct gen_cfg_t gen_cfg;
    struct pwr_cfg_t pwr_cfg;
    struct wifi_cfg_t wifi_cfg;
    struct manager_cfg_t manager_cfg;
    struct accel_cfg_t accel_cfg;
} cfg_t;

size_t safe_strlcpy(char *dest, const char *src, size_t siz);

int cfg_init(void);
cfg_t *cfg_get(void);
char *cfg_get_wifi_ssid(void);
char *cfg_get_wifi_psk(void);
char *cfg_get_mqtt_hostname(void);
float cfg_get_accel_temp_offset(void);
int32_t cfg_get_batt_mv_wifi_off(void);
int32_t cfg_get_batt_mv_wifi_on(void);
int32_t cfg_get_pv_mv_wifi_off(void);
int32_t cfg_get_pv_mv_wifi_on(void);

#endif