#ifndef __CLA_H__
#define __CLA_H__
#include "cla_pub_types.h"
#include "cla_pub_mqtt.h"

#define CLA_PUB_INITIAL_INTERVAL_MS (15000) // TODO: get from config file
#define CLA_PUB_INITIAL_INTERVAL_JITTER_MS (3000)
#define CLA_PUB_INTERVAL_MS (250) // 250 ms / 0.25 seconds
#define CLA_PUB_INTERVAL_JITTER_MS 0

// delayable work item - etern for pub scheduler
extern struct k_work_delayable cla_pub_dwork;

/**
 * @brief init cla module
 *
 * @return int
 */
int cla_pub_init(void);

/**
 * @brief get cla struct
 *
 * @return clainfo_t*
 */
int cla_pub_get(cla_pub_t *cla);

/**
 * @brief send an mqqt message with ncla data
 *
 * @return int
 */
int cla_pub(void);

#endif