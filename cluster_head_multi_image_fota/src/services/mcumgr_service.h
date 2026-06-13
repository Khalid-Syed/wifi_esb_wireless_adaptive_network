#ifndef __MCUMGR_SERVCICE_H__
#define __MCUMGR_SERVICE_H__

/**
 * @brief
 *
 * @param cb - callback to indicate service is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
int mcumgr_service_init(void(*cb), int idx);
/**
 * @brief call this when service is stalled or otherwise not healthy
 *
 */
void mcumgr_service_stall_assert(void);
#endif // __MCUMGR_SERVICE_H__