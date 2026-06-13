#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "template_pub.h"
#include "template_pub_mqtt.h"
#include "pubs.h"

//#define MODULE pubs
LOG_MODULE_DECLARE(MODULE, CONFIG_PUBS_LOG_LEVEL);

/************************************************/
/*  TEMPLATE PUB CODE                           */
/************************************************/

// work item for this pub module - use the pubs scheduler for the callback
K_WORK_DELAYABLE_DEFINE(template_pub_dwork, pubs_work_handler);

char *template_pub_state = "init";
template_pub_t template_pub_data;

int template_pub_init(void)
{
    int rc = 0;
    // do anything unique to this pub type, i.e. init some data structs etc...
    return rc;
}
int template_pub_get(template_pub_t *pdata)
{
    pdata->foo = 1;
    pdata->bar = 2;
    pdata->state = template_pub_state;
    return 0;
}
// function to fill in the template_pub_info_t struct and call mqtt publish
int template_pub(void)
{
    LOG_DBG("%s publishing!", __func__);

    // get latest data into struct
    (void)template_pub_get(&template_pub_data);
    // publish via mqtt function
    // int template_pub_mqtt_publish(&template_pub);
    return 0;
}
