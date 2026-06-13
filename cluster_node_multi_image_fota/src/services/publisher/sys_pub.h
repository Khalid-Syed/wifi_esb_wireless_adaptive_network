#ifndef __SYS_H__
#define __SYS_H__
#include "sys_pub_types.h"
#include "sys_pub_mqtt.h"

#define SYS_PUB_INITIAL_INTERVAL_MS (15000) // TODO: get from config file
#define SYS_PUB_INITIAL_INTERVAL_JITTER_MS (3000)
#define SYS_PUB_INTERVAL_MS (30 * 1000 * 60) // 30 minutes  TODO: get from config file , will want slower at scale
#define SYS_PUB_INTERVAL_JITTER_MS 1000

// delayable work item - etern for pub scheduler
extern struct k_work_delayable sys_pub_dwork;

/**
 * @brief init sys module
 *
 * @return int
 */
int sys_pub_init(void);
/**
 * @brief get sys info struct
 *
 * @param sys
 * @return int
 */
int sys_pub_get(sys_info_t *sys);
/**
 * @brief publish sys info
 *
 * @return int
 */
int sys_pub(void);

#endif