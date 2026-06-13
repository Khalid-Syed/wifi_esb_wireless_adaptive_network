#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "mppt_pub.h"
#include "mppt_pub_mqtt.h"
#include "pubs.h"

#define MODULE pubs
LOG_MODULE_DECLARE(MODULE, CONFIG_PUBS_LOG_LEVEL);

/************************************************/
/*  MPPT PUB CODE                           */
/************************************************/

// work item for this pub module - use the pubs scheduler for the callback
K_WORK_DELAYABLE_DEFINE(mppt_pub_dwork, pubs_work_handler);

int mppt_pub_init(void)
{
    int rc = 0;
    // do anything unique to this pub type, i.e. init some data structs etc...
    return rc;
}
// function to fill in the mppt_pub_info_t struct and call mqtt publish
int mppt_pub(void)
{
    LOG_DBG("%s publishing!", __func__);

    if (attempt_mppt_mtx_lock())
    {
        mppt_pub_mqtt_publish(heliostat_mppt_get());
        mppt_mtx_unlock();
    }
    return 0;
}
