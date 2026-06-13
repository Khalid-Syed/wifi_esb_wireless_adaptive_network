#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include "retained_ram.h"
#include "fatal_error_handler.h"
#include "assert_post_action.h"

static int cmd_init(const struct shell *shell, size_t argc, char **argv)
{
    bool valid = retained_ram_init();
    shell_print(shell, "retained ram was valid: %d", valid);
    return 0;
}
static int cmd_read(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "test_str: %s", retained.test_str);
    shell_print(shell, "assert_err_str: %s", retained.assert_err_str);
    shell_print(shell, "fatal_err_str: %s", retained.fatal_err_str);
    shell_print(shell, "watchdog_err_str: %s", retained.watchdog_err_str);
    shell_print(shell, "step_count_0: %d", retained.step_count_0);
    shell_print(shell, "step_count_1: %d", retained.step_count_1);
    return 0;
}
static int cmd_write(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "write noinit ram string");
    if (argc > 1)
    {
        strcpy(retained.test_str, argv[1]);
        retained_update();
    }
    else
    {
        shell_print(shell, "missing argument");
    }
    return 0;
}
static int cmd_validate(const struct shell *shell, size_t argc, char **argv)
{
    bool valid = retained_validate();
    if (valid)
    {
        shell_print(shell, "retained ram is valid");
    }
    else
    {
        shell_print(shell, "retained ram is invalid");
    }

    return 0;
}
static int cmd_erase(const struct shell *shell, size_t argc, char **argv)
{
    retained_ram_erase();
    shell_print(shell, "erased retained ram");

    return 0;
}

static int cmd_save(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "saved retained ram to files");
    int rc = retained_ram_to_files_save();
    if (rc == 0)
    {
        shell_print(shell, "saved retained ram to files");
    }
    else
    {
        shell_print(shell, "failed to save retained ram to files");
    }
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_retained,
                               SHELL_CMD(init, NULL, "init", cmd_init),
                               SHELL_CMD(read, NULL, "read", cmd_read),
                               SHELL_CMD(write, NULL, "write", cmd_write),
                               SHELL_CMD(validate, NULL, "validate", cmd_validate),
                               SHELL_CMD(erase, NULL, "erase", cmd_erase),
                               SHELL_CMD(save, NULL, "save to files", cmd_save),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(retained, &sub_retained, "noinit retained ram test cmds", NULL);
