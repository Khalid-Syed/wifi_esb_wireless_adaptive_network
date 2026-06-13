#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "ota_dfu_pub.h"
#include "pubs.h"
#include "http_update.h"

#define MODULE pubs
LOG_MODULE_DECLARE(MODULE, CONFIG_PUBS_LOG_LEVEL);

/************************************************/
/*  OTA_DFU PUB CODE                           */
/************************************************/

// work item for this pub module - use the pubs scheduler for the callback
K_WORK_DELAYABLE_DEFINE(ota_dfu_pub_dwork, pubs_work_handler);

int ota_dfu_pub_init(void)
{
    int rc = 0;
    // do anything unique to this pub type, i.e. init some data structs etc...
    return rc;
}
// function to fill in the ota_dfu_pub_info_t struct and call mqtt publish
int ota_dfu_pub(void)
{
    LOG_DBG("%s publishing!", __func__);

    // this calls into the http_update module to trigger the http download state
    // the http_update module will publish the status to mqtt, it has all of the internal state
    // info, so it's easier to call it as it does it it self while downloading.
    http_update_status_send();

    return 0;
}
