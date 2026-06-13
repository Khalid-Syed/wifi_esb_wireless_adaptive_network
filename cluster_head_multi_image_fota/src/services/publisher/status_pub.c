#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "status_pub.h"
#include "status_mqtt.h"
#include "pubs.h"

#define MODULE pubs
LOG_MODULE_DECLARE(MODULE, CONFIG_PUBS_LOG_LEVEL);

/************************************************/
/*  STATUS PUB CODE                           */
/************************************************/

// work item for this pub module - use the pubs scheduler for the callback
K_WORK_DELAYABLE_DEFINE(status_pub_dwork, pubs_work_handler);

int status_pub_init(void)
{
    int rc = 0;
    // do anything unique to this pub type, i.e. init some data structs etc...
    return rc;
}
// function to fill in the status_pub_info_t struct and call mqtt publish
int status_pub(void)
{
    LOG_DBG("%s publishing!", __func__);
    status_mqtt_publish(heliostat_status_get());
    return 0;
}
