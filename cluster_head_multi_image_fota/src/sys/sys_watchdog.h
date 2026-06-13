#ifndef __SYS_WATCHDOG_H__
#define __SYS_WATCHDOG_H__
#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/task_wdt/task_wdt.h>

/*
 * To use this, the devicetree's /aliases must have a 'watchdog0' property.
 */
#define WDT_ALLOW_CALLBACK 0
#define WDT_MAX_WINDOW 1000U
#define WDT_MIN_WINDOW 0U
#define WDT_FEED_INTERVAL 100U

#define WATCHDOG_ERROR_STR_LEN_MAX 256
#define WATCHDOG_ERROR_FILE_NAME "watchdog.json"

extern int task_wdt_id;
/**
 * @brief initilize task watchdog
 * 
 */
void sys_watchdog_init(void);
/**
 * @brief spin the watchdog to cause a reset
 * 
 */
void sys_watchdog_spin(void);
/**
 * @brief feed the watchdog
 * 
 */
static inline void sys_watchdog_feed(void)
{
#ifdef CONFIG_WATCHDOG
    task_wdt_feed(task_wdt_id);
#endif
}
/**
 * @brief save watchdog error string to file
 *
 * @param watchdog_error_msg *char
 * @return int
 */
int watchdog_error_to_file(const char *watchdog_error_msg);
/**
 * @brief read watchdog error file
 *
 * @param watchdog_error *char
 * @param len
 * @return int
 */
int watchdog_error_json_read(char *watchdog_error, int len);
#endif // __SYS_WATCHDOG_H__