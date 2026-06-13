
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#define MODULE reboot_timer
LOG_MODULE_DECLARE(MODULE, CONFIG_MAIN_LOG_LEVEL);

static void reboot_timer_expired(struct k_timer *dummy);
K_TIMER_DEFINE(reboot_timer, reboot_timer_expired, NULL);

void reboot_timer_start(int seconds)
{
    LOG_DBG("%s reboot in %d seconds", __func__, seconds);
    k_timer_start(&reboot_timer, K_SECONDS(seconds), K_NO_WAIT);
}
static void reboot_timer_expired(struct k_timer *dummy)
{
    LOG_DBG("%s", __func__);
    sys_reboot(SYS_REBOOT_WARM);
}