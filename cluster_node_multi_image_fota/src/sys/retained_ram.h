#ifndef RETAINED_RAM_H
#define RETAINED_RAM_H

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

struct retained_data
{

    char test_str[64];
    char assert_err_str[128];
    char fatal_err_str[128];
    char watchdog_err_str[192];
    int32_t step_count_0;
    int32_t step_count_1;
    /* CRC used to validate the retained data.  This must be
     * stored little-endian, and covers everything up to but not
     * including this field.
     */
    uint32_t crc;
};
extern struct retained_data retained;

#define RETAINED_CRC_OFFSET offsetof(struct retained_data, crc)
#define RETAINED_CHECKED_SIZE (RETAINED_CRC_OFFSET + sizeof(retained.crc))

/**
 * @brief Initialize retained ram
 *
 * @return true
 * @return false
 */
bool retained_ram_init(void);
/**
 * @brief Erase retained ram
 *
 * @return int
 */
int retained_ram_erase(void);
/**
 * @brief Save retained ram to files
 *
 * @return int
 */
int retained_ram_to_files_save(void);
/**
 * @brief validate integrity of retained ram
 *
 * @return true
 * @return false
 */
bool retained_validate(void);
/**
 * @brief  Update retained ram with new crc
 *
 */
void retained_update(void);

bool initial_retain_valid(void);

#endif /* RETAINED_RAM_H */