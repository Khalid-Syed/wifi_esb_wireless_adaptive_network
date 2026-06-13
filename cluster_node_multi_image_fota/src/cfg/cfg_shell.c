#include <zephyr/kernel.h>
#include <inttypes.h>

// shell commands
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include "storage.h"
#include "cfg.h"
#include "cfg_json.h"

#define MODULE cfg_shell

LOG_MODULE_REGISTER(MODULE);

static int cmd_cfg_parse(const struct shell *shell, size_t argc, char **argv)
{

    shell_print(shell, "%s", __func__);
    struct fs_file_t file;
    char path[MAX_PATH_LEN];
    char json_str[1024];
    cfg_t cfg;

    if (argc < 2)
    {
        shell_print(shell, "error  argc = %d", argc);
        return 0;
    }
    struct fs_mount_t *mp = storage_mount_point_get();

    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, argv[1]);
    shell_print(shell, "parse %s", path);

    fs_file_t_init(&file);
    int rc = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: open %s: %d", path, rc);
        goto out;
    }
    rc = fs_read(&file, json_str, sizeof(json_str));
    if (rc < 0)
    {
        shell_error(shell, "FAIL: read %s: [rd:%d]", path, rc);
        goto out;
    }
    json_str[rc] = 0; // null terminate
    shell_print(shell, "parsing json_str: %s", json_str);
    rc = cfg_decode(json_str, &cfg);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: cfg_decode %s: [rc:%d]", path, rc);
        goto out;
    }
    log_cfg(cfg_get());
out:
    return 0;
}
static int cmd_cfg_init(const struct shell *shell, size_t argc, char **argv)
{
    cfg_init();
    return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_cfg,
                               SHELL_CMD(init, NULL, "init", cmd_cfg_init),
                               SHELL_CMD(parse, NULL, "parse [file]", cmd_cfg_parse),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(cfg, &sub_cfg, "cfg", NULL);
