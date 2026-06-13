
#include <stddef.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include "sys_watchdog.h"
#include "retained_ram.h"
#include "string_utils.h"
#include "storage.h"
#include "boot_count.h"

#define MODULE sys_wdt
LOG_MODULE_REGISTER(MODULE, CONFIG_SYS_WDT_LOG_LEVEL);

int task_wdt_id;
const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
char thread_name[64] = {0};

/* Nordic supports a callback, but it has 61.2 us to complete before
 * the reset occurs, which is too short to do anything with serial flash
 */

static void task_wdt_callback(int channel_id, void *user_data)
{
#if CONFIG_WATCHDOG
    // LOG_DBG("Task watchdog channel %d callback, thread: %s",
    //         channel_id, k_thread_name_get((k_tid_t)user_data));

    /*
     * If the issue could be resolved, call task_wdt_feed(channel_id) here
     * to continue operation.
     *
     * Otherwise we can perform some cleanup and reset the device.
     */

    task_wdt_feed(channel_id);
    /* Disable interrupts, this is unrecoverable */
    (void)irq_lock();

    safe_strlcpy(thread_name, k_thread_name_get((k_tid_t)user_data), sizeof(thread_name));

    // save watchdog channel id to retained ram - cant write to serial flash in this context
    sprintf(retained.watchdog_err_str, "{\"error\":\"watchdog\","
                                       "\"channel_id\":\"%d\","
                                       "\"thread\":\"%s\","
                                       "\"boot_count\":%d,"
                                       "\"uptime\":%lld"
                                       "}",
            channel_id,
            thread_name,
            boot_count_get(),
            k_uptime_get() / 1000);

    retained_update();
    LOG_ERR("Resetting device...");

    sys_reboot(SYS_REBOOT_COLD);
#endif
}
/**
 * @brief initilize task watchdog
 *
 */
void sys_watchdog_init(void)
{
#if CONFIG_WATCHDOG

    if (!device_is_ready(wdt))
    {
        LOG_ERR("Hardware watchdog not ready; ignoring it");
    }

    int ret = task_wdt_init(wdt);
    if (ret != 0)
    {
        LOG_ERR("task wdt init failure: %d", ret);
        return;
    }

    /*
     * Add a new task watchdog channel with custom callback function and
     * the current thread ID as user data.
     */
    task_wdt_id = task_wdt_add(CONFIG_TASK_WDT_MIN_TIMEOUT, task_wdt_callback,
                               (void *)k_current_get());
#endif
}
/**
 * @brief spin the watchdog to cause a reset
 *
 */
void sys_watchdog_spin(void)
{
#if CONFIG_WATCHDOG
    /* Waiting for the SoC reset. */
    LOG_INF("Waiting for reset...\n");
    /* Disable interrupts, this is unrecoverable */
    (void)irq_lock();
    while (1)
    {
        ;
    }
#else
    LOG_ERR("%s Watchdog not enabled", __func__);
#endif
}

/**
 * @brief save watchdog error string to file
 *
 * @param watchdog_error_msg *char
 * @return int
 */
int watchdog_error_to_file(const char *watchdog_error_msg)
{
    struct fs_file_t watchdog_file;
    char path[MAX_PATH_LEN];

    struct fs_mount_t *mp = storage_mount_point_get();
    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, "watchdog.json");
    fs_unlink(path); // delete old watchdog file if it exists

    fs_file_t_init(&watchdog_file);

    int rc = fs_open(&watchdog_file, path, FS_O_CREATE | FS_O_RDWR);
    if (rc >= 0)
    {
        fs_write(&watchdog_file, watchdog_error_msg, strlen(watchdog_error_msg));
        if (rc < 0)
        {
            LOG_ERR("fail to write watchdog error file rc = %d", rc);
        }
        fs_close(&watchdog_file);
    }
    return rc;
}

/**
 * @brief read watchdog error file
 *
 * @param watchdog_error *char
 * @param len
 * @return int
 */
int watchdog_error_json_read(char *watchdog_error, int len)
{
    struct fs_file_t watchdog_error_file;
    char path[MAX_PATH_LEN];
    int rc;

    struct fs_mount_t *mp = storage_mount_point_get();
    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, WATCHDOG_ERROR_FILE_NAME);
    fs_file_t_init(&watchdog_error_file);
    rc = fs_open(&watchdog_error_file, path, FS_O_READ);
    if (rc < 0)
    {
        LOG_INF("no watchdog error file");
        return rc;
    }
    else
    {
        rc = fs_read(&watchdog_error_file, watchdog_error, len);
        if (rc < 0)
        {
            LOG_ERR("fail to read watchdog error file rc = %d", rc);
        }
    }
    fs_close(&watchdog_error_file);
    return rc;
}
