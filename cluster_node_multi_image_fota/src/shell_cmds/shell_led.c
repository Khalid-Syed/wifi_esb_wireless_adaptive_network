#include <stdlib.h>
#include <string.h>
#include "led_patterns.h"
#include "led_timer.h"
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>


static int cmd_led_pattern_list(const struct shell *shell, size_t argc, char **argv)
{
    for (int i = 0; i < led_patterns_count_get(); i++)
    {
        shell_print(shell, "%d:  %s", i, led_patterns_get()[i]->name);
    }
    return 0;
}
static int cmd_led_pattern_start(const struct shell *shell, size_t argc, char **argv)
{
    if (argc <= 1)
    {
        shell_print(shell, "usage: led start <pattern number>");
        return 0;
    }
    if (atoi(argv[1]) < led_patterns_count_get() && atoi(argv[1]) >= 0)
    {
        int idx = atoi(argv[1]);
        shell_print(shell, "starting pattern %d. %s", idx, led_patterns_get()[idx]->name);
        led_timer_start(led_patterns_get()[idx]);
    }
    else
    {
        shell_print(shell, "invalid pattern number");
    }
    return 0;
}

static int cmd_led_pattern_stop(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1)
    {
        shell_print(shell, "usage: led stop");
        led_timer_stop(led_timer_get_state()->pattern);
    }
    int idx = atoi(argv[1]);
    if (idx < led_patterns_count_get() && idx >= 0)
    {
        led_timer_stop(led_patterns_get()[idx]);
    }
    else
    {
        shell_print(shell, "invalid pattern number");
    }

    return 0;
}
static int cmd_led_on(const struct shell *shell, size_t argc, char **argv)
{
    led0_set(true);
    return 0;
}
static int cmd_led_off(const struct shell *shell, size_t argc, char **argv)
{
    led0_set(false);
    return 0;
}
static int cmd_led_timer_enable(const struct shell *shell, size_t argc, char **argv)
{
    led_timer_enable();
    return 0;
}
static int cmd_led_timer_disable(const struct shell *shell, size_t argc, char **argv)
{
    led_timer_disable();
    return 0;
}

//
SHELL_STATIC_SUBCMD_SET_CREATE(sub_led,
                               SHELL_CMD(list, NULL, "list led patterns", cmd_led_pattern_list),
                               SHELL_CMD(start, NULL, "start led pattern [x]", cmd_led_pattern_start),
                               SHELL_CMD(stop, NULL, "stop led pattern [x]", cmd_led_pattern_stop),
                               SHELL_CMD(on, NULL, "turn led on", cmd_led_on),
                               SHELL_CMD(off, NULL, "turn led off", cmd_led_off),
                               SHELL_CMD(enable, NULL, "enable led timer", cmd_led_timer_enable),
                               SHELL_CMD(disable, NULL, "disable led timer", cmd_led_timer_disable),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(led, &sub_led, "led commands", NULL);