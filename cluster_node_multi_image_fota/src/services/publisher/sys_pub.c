#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include "sys_pub.h"
#include "sys_pub_mqtt.h"
#include "app_version.h"
#include "boot_count.h"
#include "reset_cause.h"
#include "assert_post_action.h"
#include "fatal_error_handler.h"
#include "pubs.h"
#include "transport.h"

#define MODULE pubs
LOG_MODULE_DECLARE(MODULE, CONFIG_PUBS_LOG_LEVEL);

K_WORK_DELAYABLE_DEFINE(sys_pub_dwork, pubs_work_handler);

static sys_info_t sys;
static char assert_json_str[ASSERT_STR_LEN_MAX] = "{\"file\":\"no assert\",\"line\":0,\"boot_count\":0,\"uptime\":0}";
static char fatal_error_json_str[FATAL_ERROR_STR_LEN_MAX] = "{\"error\":\"none\",\"reason\":\"NA\",\"boot_count\":0,\"uptime\":0}";

int sys_pub_init(void)
{
    int rc = assert_json_read(assert_json_str, ASSERT_STR_LEN_MAX);
    if (rc < 0)
    {
        LOG_ERR("Failed to read assert file, rc = %d", rc);
    }
    else
    {
        LOG_DBG("assert str: \"%s\"", assert_json_str);
    }

    rc = fatal_error_json_read(fatal_error_json_str, FATAL_ERROR_STR_LEN_MAX);
    if (rc < 0)
    {
        LOG_ERR("Failed to read fatal error file, rc = %d", rc);
    }
    else
    {
        LOG_DBG("fatal err str: \"%s\"", fatal_error_json_str);
    }
    return rc;
}
int sys_pub_get(sys_info_t *sys)
{
    transport_stats_t tstats;
    sys->uptime = k_uptime_get() / 1000; // report uptime in seconds
    sys->boot_count = boot_count_get();
    sys->reboot_cause = reset_cause_get();
    sys->assert_json = assert_json_str;
    sys->fatal_error_json = fatal_error_json_str;

    (void)transport_stats_get(&tstats);
    sys->con_s = tstats.con_s;
    if (transport_state_ready())
    {
        sys->con_status = TRANSPORT_STATUS_EVENT_READY;
    }
    else
    {
        sys->con_status = TRANSPORT_STATUS_EVENT_DISCONNECTED;
    }
    return 0;
}
int sys_pub(void)
{
    LOG_DBG("");
    if (sys_pub_get(&sys) == 0)
    {
        sys_pub_mqtt_publish(&sys);
    }
    return 0;
}
