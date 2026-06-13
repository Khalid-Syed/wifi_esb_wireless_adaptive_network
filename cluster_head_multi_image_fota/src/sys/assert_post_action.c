#include <stdio.h>
#include <string.h>
#include <nrfx_clock.h>
#include <nrfx_gpiote.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include "storage.h"
#include "boot_count.h"
#include "assert_post_action.h"
#include "retained_ram.h"
#include "string_utils.h"

#define MODULE main
LOG_MODULE_DECLARE(MODULE, CONFIG_MAIN_LOG_LEVEL);

static int default_assert_string(char *assert_str);

// assert support
void assert_post_action(const char *file, unsigned int line)
{
    char assert_msg[ASSERT_STR_LEN_MAX];

    /* Disable interrupts, this is unrecoverable */
    (void)irq_lock();

    // TODO: [G5FW-140] if the uptime is less than a certain amount of time, we could tell MCUBoot to roll back

    // TODO: [G5FW-141] if the device is rebooting over and over, it would be good to not wear our flash out
    // by writing the same assert message over and over.  We could check the last assert message
    // and only write if it's different.  Also could check the uptime and only write if it's been
    // more than a minute or so.    fs_write(&assert_file, assert_msg, strlen(assert_msg));

    // create JSON string with assert info, previous boot count, and system uptime
    sprintf(assert_msg, "{\"file\":\"%s\","
                        "\"line\":%d,"
                        "\"boot_count\":%d,"
                        "\"uptime\":%lld"
                        "}",
            file,
            line,
            boot_count_get(),
            k_uptime_get() / 1000);
    // save assert message to retained ram
    safe_strlcpy(retained.assert_err_str, assert_msg, sizeof(retained.assert_err_str));

    retained_update();
    // reboot
    sys_reboot(SYS_REBOOT_WARM);
    CODE_UNREACHABLE;
    return;
}

// write assert message to file
int assert_string_to_file(const char *assert_msg)
{
    storage_remove(ASSERT_FILE_NAME); // delete old assert file if it exists
    int rc = storage_write(ASSERT_FILE_NAME, assert_msg, strlen(assert_msg) + 1);
    if (rc < 0)
    {
        LOG_ERR("fail to write assert file rc = %d", rc);
    }
    return rc;
}
int assert_json_read(char *assert, int len)
{
    char tmp[ASSERT_STR_LEN_MAX] = {0};
    int default_len;
    int rc;

    rc = storage_read(ASSERT_FILE_NAME, 0, tmp, ASSERT_STR_LEN_MAX);
    if (rc < 0)
    {
        LOG_DBG("no assert file - create default");
        storage_remove(ASSERT_FILE_NAME);
        default_len = default_assert_string(tmp);
        rc = storage_write(ASSERT_FILE_NAME, tmp, default_len);
        safe_strlcpy(assert, tmp, len);
        return rc;
    }
    else
    {
        if (rc > 0 && is_ascii_str(tmp, len))
        {
            LOG_DBG("yay! assert file is ascii and non-null");
            safe_strlcpy(assert, tmp, len); // copy to caller buffer
        }
        else
        {
            LOG_ERR("assert file is not ascii or is empty, resetting to default");
            storage_remove(ASSERT_FILE_NAME);
            default_len = default_assert_string(tmp);
            rc = storage_write(ASSERT_FILE_NAME, tmp, default_len);
            safe_strlcpy(assert, tmp, len);
        }
    }
    LOG_DBG("assert json:\"%s\"", assert);
    return rc;
}
static int default_assert_string(char *assert_str)
{
    int len = sprintf(assert_str, "{\"file\":\"%s\","
                                  "\"line\":%d,"
                                  "\"boot_count\":%d,"
                                  "\"uptime\":%d"
                                  "}",
                      "",
                      0,
                      0,
                      0);

    return len + 1;
}