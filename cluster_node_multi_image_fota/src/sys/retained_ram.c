
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include "retained_ram.h"
#include "fatal_error_handler.h"
#include "assert_post_action.h"
#include "sys_watchdog.h"

#include <zephyr/logging/log.h>
#define MODULE sys
LOG_MODULE_REGISTER(sys, CONFIG_SYS_LOG_LEVEL);
/* Retained data must be defined in a no-init section to prevent the C
 * runtime initialization from zeroing it before anybody can see it.
 */
__noinit struct retained_data retained;

static volatile bool init_retain_valid = false;

/**
 * @brief validate integrity of retained ram
 *
 * @return true
 * @return false
 */
bool retained_validate(void)
{
    /* The residue of a CRC is what you get from the CRC over the
     * message catenated with its CRC.  This is the post-final-xor
     * residue for CRC-32 (CRC-32/ISO-HDLC) which Zephyr calls
     * crc32_ieee.
     */
    const uint32_t residue = 0x2144df1c;
    uint32_t crc = crc32_ieee((const uint8_t *)&retained,
                              RETAINED_CHECKED_SIZE);
    bool valid = (crc == residue);

    /* If the CRC isn't valid, reset the retained data. */
    if (!valid)
    {
        memset(&retained, 0, sizeof(retained));
    }

    /* Reset to accrue runtime from this session. */
    // retained.uptime_latest = 0;

    /* Reconfigure to retain the state during system off, regardless of
     * whether validation succeeded.  Although these values can sometimes
     * be observed to be preserved across System OFF, the product
     * specification states they are not retained in that situation, and
     * that can also be observed.
     */
    //(void)ram_range_retain(&retained, RETAINED_CHECKED_SIZE, true);

    return valid;
}
/**
 * @brief  Update retained ram with new crc
 *
 */
void retained_update(void)
{

    uint32_t crc = crc32_ieee((const uint8_t *)&retained,
                              RETAINED_CRC_OFFSET);

    retained.crc = sys_cpu_to_le32(crc);
}
/**
 * @brief Save retained ram to files
 *
 * @return int
 */
int retained_ram_to_files_save(void)
{
    if (strlen(retained.fatal_err_str) > 0)
    {
        LOG_INF("fatal_err_str: %s", retained.fatal_err_str);
        fatal_error_to_file(retained.fatal_err_str);
    }
    if (strlen(retained.assert_err_str) > 0)
    {
        LOG_INF("assert_err_str: %s", retained.assert_err_str);
        assert_string_to_file(retained.assert_err_str);
    }
    if (strlen(retained.watchdog_err_str) > 0)
    {
        LOG_INF("watchdog_err_str: %s", retained.watchdog_err_str);
        watchdog_error_to_file(retained.watchdog_err_str);
    }

    return 0;
}
/**
 * @brief Erase retained ram
 *
 * @return int
 */
int retained_ram_erase(void)
{
    memset(&retained, 0, sizeof(retained));
    retained_update(); // fix crc
    return 0;
}

bool initial_retain_valid(void)
{
    return init_retain_valid;
}

/**
 * @brief Initialize retained ram
 *
 * @return true
 * @return false
 */
bool retained_ram_init(void)
{
    bool valid = retained_validate();
    if (valid)
    {
        LOG_INF("retained ram is valid");
        init_retain_valid = true;
    }
    else
    {
        LOG_INF("retained ram is invalid");
        LOG_INF("erase retained ram");
        init_retain_valid = false;
        retained_update(); // fix crc
    }

    return valid;
}