
#include <zephyr/kernel.h>
#include "storage.h"
#include "cfg.h"
#include "cfg_defaults.h"
#include "manager_connectivity_monitor.h"

// the config struct is defined in cfg.h

int cfg_defaults(cfg_t *cfg_p)
{
    cfg_p->gen_cfg.v = "default";
    cfg_p->pwr_cfg.is_prod_unit = false;
    cfg_p->pwr_cfg.batt_mv_wifi_off = 3000;
    cfg_p->pwr_cfg.batt_mv_wifi_on = 3100;
    cfg_p->pwr_cfg.pv_mv_wifi_off = 1000;
    cfg_p->pwr_cfg.pv_mv_wifi_on = 1250;
    cfg_p->wifi_cfg.ssid = CONFIG_HELIOGEN_SSID_24GHZ;
    cfg_p->wifi_cfg.psk = CONFIG_HELIOGEN_PASSWORD;
    cfg_p->wifi_cfg.mqtt_hn = CONFIG_MQTT_TRANSPORT_BROKER_HOSTNAME;
    cfg_p->manager_cfg.trd_mon_en = true;
    cfg_p->manager_cfg.wifi_to_ms = MANAGER_CONNECTIVITY_MONITOR_WIFI_TIMEOUT_MS;
    cfg_p->manager_cfg.mqtt_to_ms = MANAGER_CONNECTIVITY_MONITOR_TRANSPORT_TIMEOUT_MS;
    cfg_p->accel_cfg.temp_offset = "25.0";
    return 0;
}
