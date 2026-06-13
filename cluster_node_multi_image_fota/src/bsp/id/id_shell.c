/*
serial number shell commands
*/
#include <zephyr/random/rand32.h>
#include <hw_id.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include "./id.h"
/**
 * @brief show hardware id
 *
 * @param shell
 * @param argc
 * @param argv
 * @return int
 */
static int cmd_id(const struct shell *shell, size_t argc, char **argv)
{
    char id[48];

    if (!id_get(id, sizeof(id)))
    {
        shell_print(shell, "client id: %s", id);
    }
    else
    {
        shell_print(shell, "client id: %s", "error");
    }
    return 0;
}

SHELL_CMD_ARG_REGISTER(sn, NULL, "Show hardware id", cmd_id, 1, 0);