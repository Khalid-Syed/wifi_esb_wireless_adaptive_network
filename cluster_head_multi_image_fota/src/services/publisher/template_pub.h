#ifndef __TEMPLATE_PUB_H__
#define __TEMPLATE_PUB_H__
#include "template_pub_types.h" // defines the data struct to publish
#include "template_pub_mqtt.h"  // defines the topic and mqtt publish function

#define TEMPLATE_PUB_INITIAL_INTERVAL_MS (15000) // TODO: get from config file
#define TEMPLATE_PUB_INITIAL_INTERVAL_JITTER_MS (3000)
#define TEMPLATE_PUB_INTERVAL_MS (15 * 1000n * 60) // 15 minutes  TODO: get from config file , will want slower at scale
#define TEMPLATE_PUB_INTERVAL_JITTER_MS 1000

// delayable work item
extern struct k_work_delayable template_pub_dwork;

/**
 * @brief init template pub
 *
 * @return int
 */
int template_pub_init(void);
/**
 * @brief get template struct
 *
 * @param void *pdata
 * @return int
 */
int template_pub_get(template_pub_t *pdata);

/**
 * @brief publish template data now
 *
 * @return int
 */
int template_pub(void);

#endif