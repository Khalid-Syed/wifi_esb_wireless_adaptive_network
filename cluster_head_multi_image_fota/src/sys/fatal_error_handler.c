#include <stdio.h>
#include <string.h>
#include <nrfx_clock.h>
#include <nrfx_gpiote.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/fatal.h>
#include "storage.h"
#include "boot_count.h"
#include "led.h"
#include "retained_ram.h"
#include "fatal_error_handler.h"
#include "string_utils.h"

#define MODULE main
LOG_MODULE_DECLARE(MODULE, CONFIG_MAIN_LOG_LEVEL);

char *k_fatal_error_reason_strs[] = {
    /** Generic CPU exception, not covered by other codes */
    "K_ERR_CPU_EXCEPTION",
    /** Unhandled hardware interrupt */
    "K_ERR_SPURIOUS_IRQ",
    /** Faulting context overflowed its stack buffer */
    "K_ERR_STACK_CHK_FAIL",
    /** Moderate severity software error */
    "K_ERR_KERNEL_OOPS",
    /** High severity software error */
    "K_ERR_KERNEL_PANIC"};

static void led_blinky(int count);
static int default_fatal_error_string(char *fatal_error_str);

/**
 * @brief fatal error handler - we are able to ovveride the default weak handler
 *           with this if CONFIG_RESET_ON_FATAL_ERROR=n
 *
 * @param reason
 * @param esf
 */
void k_sys_fatal_error_handler(unsigned int reason,
                               const z_arch_esf_t *esf)
{
    char fatal_msg[FATAL_ERROR_STR_LEN_MAX];

    /* Disable interrupts, this is unrecoverable */
    (void)irq_lock();

    // save fatal error to retained ram - cant write to serial flash in this context
    sprintf(retained.fatal_err_str, "{\"error\":\"fatal\","
                                    "\"reason\":\"%s\","
                                    "\"boot_count\":%d,"
                                    "\"uptime\":%lld"
                                    "}",
            k_fatal_error_reason_strs[reason],
            boot_count_get(),
            k_uptime_get() / 1000);

    safe_strlcpy(retained.fatal_err_str, fatal_msg, sizeof(retained.fatal_err_str));

    retained_update();
    // blink led then reboot
    led_blinky(10);
    sys_reboot(SYS_REBOOT_COLD);
    CODE_UNREACHABLE;
}
/**
 * @brief save fatal error string to file
 *
 * @param fatal_error_msg *char
 * @return int
 */
int fatal_error_to_file(const char *fatal_error_msg)
{

    storage_remove(FATAL_ERROR_FILE_NAME); // delete old fatal file if it exists
    int rc = storage_write(FATAL_ERROR_FILE_NAME, fatal_error_msg, strlen(fatal_error_msg) + 1);
    if (rc < 0)
    {
        LOG_ERR("fail to write fatal error file rc = %d", rc);
    }
    return rc;
}
/**
 * @brief read fatal error file
 *
 * @param fatal_error *char
 * @param len
 * @return int
 */
int fatal_error_json_read(char *fatal_error, int len)
{
    char tmp[FATAL_ERROR_STR_LEN_MAX] = {0};
    int default_len;
    int rc;

    rc = storage_read(FATAL_ERROR_FILE_NAME, 0, tmp, FATAL_ERROR_STR_LEN_MAX);
    if (rc < 0)
    {
        LOG_DBG("no fatal error file - create default");
        storage_remove(FATAL_ERROR_FILE_NAME);
        default_len = default_fatal_error_string(tmp);
        rc = storage_write(FATAL_ERROR_FILE_NAME, tmp, default_len);
        safe_strlcpy(fatal_error, tmp, len);
        return rc;
    }
    else
    {
        if (rc > 0 && is_ascii_str(tmp, len))
        {
            LOG_DBG("yay! fatal error file is ascii and non-null");
            safe_strlcpy(fatal_error, tmp, len); // copy to caller buffer
        }
        else
        {
            LOG_ERR("fatal error file is not ascii or is empty, resetting to default");
            storage_remove(FATAL_ERROR_FILE_NAME);
            default_len = default_fatal_error_string(tmp);
            rc = storage_write(FATAL_ERROR_FILE_NAME, tmp, default_len);
            safe_strlcpy(fatal_error, tmp, len);
        }
    }
    LOG_DBG("fatal error json:\"%s\"", fatal_error);
    return rc;
}

static void led_blinky(int count)
{
    led0_init();
    for (int i = 0; i < count; i++)
    {
        led0_set(true);
        k_busy_wait(100000);
        led0_set(false);
        k_busy_wait(100000);
    }
}
// TODO: [G5FW-218]check fatal error and assert strings for valid ascii
static int default_fatal_error_string(char *fatal_error_str)
{

    int len = sprintf(fatal_error_str, "{\"error\":\"none\","
                                       "\"reason\":\"none\","
                                       "\"boot_count\":0,"
                                       "\"uptime\":0 }");
    return len + 1;
}