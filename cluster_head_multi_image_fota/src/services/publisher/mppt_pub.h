#ifndef __MPPT_PUB_H__
#define __MPPT_PUB_H__
#include "mppt_pub_types.h" // defines the data struct to publish
#include "mppt_pub_mqtt.h"  // defines the topic and mqtt publish function
#include "power_obj.h"

#define MPPT_PUB_INITIAL_INTERVAL_MS (15000) // TODO: get from config file
#define MPPT_PUB_INITIAL_INTERVAL_JITTER_MS (3000)
#define MPPT_PUB_INTERVAL_MS (30 * 1000 * 60) // 30 minutes  TODO: get from config file , will want slower at scale
#define MPPT_PUB_INTERVAL_JITTER_MS 1000

// delayable work item
extern struct k_work_delayable mppt_pub_dwork;

/**
 * @brief init mppt pub
 *
 * @return int
 */
int mppt_pub_init(void);
/**
 * @brief get mppt struct
 *
 * @param void *pdata
 * @return int
 */
int mppt_pub_get(mppt_pub_t *pdata);

/**
 * @brief publish mppt data now
 *
 * @return int
 */
int mppt_pub(void);

#endif