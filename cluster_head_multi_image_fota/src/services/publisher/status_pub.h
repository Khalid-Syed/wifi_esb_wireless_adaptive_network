#ifndef __STATUS_PUB_H__
#define __STATUS_PUB_H__
#include "status_mqtt.h" // defines the topic and mqtt publish function

#define STATUS_PUB_INITIAL_INTERVAL_MS (15000) // TODO: get from config file
#define STATUS_PUB_INITIAL_INTERVAL_JITTER_MS (3000)
#define STATUS_PUB_INTERVAL_MS (30 * 1000 * 60) // 30 minutes  TODO: get from config file , will want slower at scale
#define STATUS_PUB_INTERVAL_JITTER_MS 1000

// delayable work item
extern struct k_work_delayable status_pub_dwork;

/**
 * @brief init status pub
 *
 * @return int
 */
int status_pub_init(void);

/**
 * @brief publish status data now
 *
 * @return int
 */
int status_pub(void);

#endif