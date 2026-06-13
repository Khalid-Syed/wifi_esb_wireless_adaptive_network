
#include <stddef.h>
#include <stdlib.h>

#include "motor_constants.h"
#include "status_json.h"
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#define MODULE manager_shell

LOG_MODULE_REGISTER(MODULE);

#include "manager_thread_monitor.h"
static int cmd_manager_timeout(const struct shell *shell, size_t argc, char **argv)
{
    int32_t idx = atoi(argv[1]);
    shell_print(shell, "artificially causing thread %d to timeout", idx);
    thread_monitor_timeout_artificial(idx);
    return 0;
}
#include "manager_connectivity_monitor.h"
static int cmd_manager_connectivity_timeout_wifi(const struct shell *shell, size_t argc, char **argv)
{
    int ms = atoi(argv[1]);
    shell_print(shell, "set wifi timeout to %d", ms);
    manager_connectivity_monitor_wifi_timeout_set(ms);
    return 0;
}
static int cmd_manager_connectivity_timeout_transport(const struct shell *shell, size_t argc, char **argv)
{
    int ms = atoi(argv[1]);
    shell_print(shell, "set transport timeout to %d", ms);
    manager_connectivity_monitor_transport_timeout_set(ms);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_manager,
                               SHELL_CMD(thread_timeout, NULL, "artifically cause a thread to timeout and assert", cmd_manager_timeout),
                               SHELL_CMD(wifi_timeout, NULL, "set wifi connection timeout", cmd_manager_connectivity_timeout_wifi),
                               SHELL_CMD(transport_timeout, NULL, "set transport connection timeout", cmd_manager_connectivity_timeout_transport),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(manager, &sub_manager, "Manager commands", NULL);
