
#include "id.h"
/**
 * @brief Get the hardware id of the device
 *
 * @param buffer
 * @param buffer_size
 * @return int
 */
int id_get(char *const buffer, size_t buffer_size)
{
	int ret;
	// use zephyr api to get mac address or serial number
	ret = hw_id_get(buffer, buffer_size);
	if (ret)
	{
		return ret;
	}

	return 0;
}
