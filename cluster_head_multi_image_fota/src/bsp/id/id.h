
#ifndef ID_H__
#define ID_H__
#include <zephyr/types.h>
#include <hw_id.h>

/**
 * @brief Get the hardware id of the device
 * @details This function is used to get the hardware id of the device.
 *          Use CONFIG_HW_ID_LIBRARY_SOURCE_DEVICE_ID=y to use nordic serial number
 *          Use CONFIG_HW_ID_LIBRARY_SOURCE_NET_MAC=y to use nordic wi-fi mac address
 * @param buffer
 * @param buffer_size
 * @return int
 */
int id_get(char *const buffer, size_t buffer_size);
#define ID_LEN HW_ID_LEN
#endif