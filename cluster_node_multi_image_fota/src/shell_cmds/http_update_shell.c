
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include "http_update.h"

static int shell_update(const struct shell *shell, size_t argc, char **argv)
{
    char *install;
    int retries;

    if (argc < 2)
    {
        shell_print(shell, "error  argc = %d", argc);
        return 0;
    }
    shell_print(shell, "update host = %s, filename = %s", argv[1], argv[2]);
    if (argc == 2)
    {
        install = "now";
        retries = 0;
    }
    else if (argc == 3)
    {
        install = argv[3];
        retries = 0;
    }
    else
    {
        install = argv[3];
        retries = atoi(argv[4]);
    }
    // use default session id 0
    http_update_start(0, argv[1], argv[2], install, retries);
    return 0;
}

SHELL_CMD_REGISTER(update, NULL, "update [uri] [file] (install_mode: now/wait/stow) (retries)", shell_update);
