#ifndef __WIFI_THREAD_H__
#define __WIFI_THREAD_H__
/**
 * @brief
 *
 * @param cb - callback to indicate thread is healthy
 * @param idx - index to pass back to im_alive callback
 * @return k_tid_t
 */
k_tid_t wifi_thread_start(void(*cb), int idx);
/**
 * @brief call this when thread is stalled or otherwise not healthy
 *
 */
void wifi_thread_stall_assert(void);

extern bool fetch_server_time_success;

#endif // __WIFI_THREAD_H__