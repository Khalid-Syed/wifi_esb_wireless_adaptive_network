#ifndef __OTA_DFU_PUB_H__
#define __OTA_DFU_PUB_H__
#include "ota_dfu_mqtt.h" // defines the topic and mqtt publish function

#define OTA_DFU_PUB_INITIAL_INTERVAL_MS (15000) // TODO: get from config file
#define OTA_DFU_PUB_INITIAL_INTERVAL_JITTER_MS (3000)
#define OTA_DFU_PUB_INTERVAL_MS (30 * 1000 * 60) // 30 minutes  TODO: get from config file , will want slower at scale
#define OTA_DFU_PUB_INTERVAL_JITTER_MS 1000

// delayable work item
extern struct k_work_delayable ota_dfu_pub_dwork;

/**
 * @brief init ota_dfu pub
 *
 * @return int
 */
int ota_dfu_pub_init(void);

/**
 * @brief publish ota_dfu data now
 *
 * @return int
 */
int ota_dfu_pub(void);

#endif