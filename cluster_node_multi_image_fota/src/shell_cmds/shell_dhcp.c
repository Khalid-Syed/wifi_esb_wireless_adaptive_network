
// shell commands
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/net/net_if.h>
#include "zephyr/net/dhcpv4.h"
static int cmd_dhcp_restart(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 1)
    {
        shell_error(shell, "missing argument");
    }
    else if (!strcmp(argv[1], "restart"))
    {
        shell_print(shell, "restarting DHCP");
        net_dhcpv4_restart(net_if_get_default());
    }
    return 0;
}
SHELL_CMD_REGISTER(dhcp, NULL, "Restart DHCP", cmd_dhcp_restart);