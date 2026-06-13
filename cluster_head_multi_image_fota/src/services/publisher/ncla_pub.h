#ifndef __NCLA_H__
#define __NCLA_H__
#include "ncla_pub_types.h"
#include "ncla_pub_mqtt.h"

#define NCLA_PUB_INITIAL_INTERVAL_MS (0) // TODO: get from config file
#define NCLA_PUB_INITIAL_INTERVAL_JITTER_MS (0)
#define NCLA_PUB_INTERVAL_MS (250) // 250 milliseconds
#define NCLA_PUB_INTERVAL_JITTER_MS 0

// delayable work item - etern for pub scheduler
extern struct k_work_delayable ncla_pub_dwork;

/**
 * @brief init ncla module
 *
 * @return int
 */
int ncla_pub_init(void);

/**
 * @brief get ncla struct
 *
 * @return nclainfo_t*
 */
int ncla_pub_get(ncla_pub_t *ncla);

/**
 * @brief send an mqqt message with ncla data
 *
 * @return int
 */
int ncla_pub(void);

#endif