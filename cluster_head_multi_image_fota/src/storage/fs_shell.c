
// shell commands for playing with the file system

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include "storage.h"
#include "storage_counter.h"

#define MODULE fs_shell

LOG_MODULE_REGISTER(module, CONFIG_FS_LOG_LEVEL);

extern void fs_test(const struct shell *);

#define DIR_LEN_MAX 1024
static int cmd_fs_ls(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    char list[DIR_LEN_MAX];
    list[0] = '\0';

    shell_print(shell, "%s", __func__);
    int len = storage_list(list, DIR_LEN_MAX);
    shell_print(shell, "ls len: %d\n%s", len, list);
    return 0;
}
static int cmd_fs_stats(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    struct fs_statvfs sbuf;

    shell_print(shell, "%s", __func__);
    struct fs_mount_t *mp = storage_mount_point_get();
    int rc = fs_statvfs(mp->mnt_point, &sbuf);
    if (rc < 0)
    {
        shell_print(shell, "FAIL: statvfs: %d", rc);
        return 0;
    }

    shell_print(shell, "%s: bsize = %lu ; frsize = %lu ;"
                       " blocks = %lu ; bfree = %lu",
                mp->mnt_point,
                sbuf.f_bsize, sbuf.f_frsize,
                sbuf.f_blocks, sbuf.f_bfree);

    return 0;
}

#define CHUNK_SIZE 64
static int cmd_fs_read(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2)
    {
        shell_print(shell, "error  argc = %d", argc);
        return 0;
    }
    shell_print(shell, "read %s:", argv[1]);

    char data[CHUNK_SIZE];
    int offset = 0;
    int rc;
    // read from the file a chunk at a time
    while (1)
    {
        shell_print(shell, "offset = %d", offset);
        rc = storage_read(argv[1], offset, data, CHUNK_SIZE);
        shell_print(shell, "read = %d bytes", rc);
        if (rc < 0)
        {
            shell_error(shell, "FAIL: read %s: [rd:%d]", argv[1], rc);
            break;
        }
        shell_hexdump(shell, data, rc);
        if (rc < sizeof(data))
        { // reached EOF
            break;
        }
        offset += rc;
    }
    return 0;
}
#define ASCII_CHUNK_SIZE 1024
static int cmd_fs_read_ascii(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2)
    {
        shell_print(shell, "error  argc = %d", argc);
        return 0;
    }
    shell_print(shell, "read %s:", argv[1]);

    char data[ASCII_CHUNK_SIZE];
    int offset = 0;
    int rc;
    // read from the file a chunk at a time
    while (1)
    {
        shell_print(shell, "offset = %d", offset);
        rc = storage_read(argv[1], offset, data, CHUNK_SIZE);
        shell_print(shell, "read = %d bytes", rc);
        if (rc < 0)
        {
            shell_error(shell, "FAIL: read %s: [rd:%d]", argv[1], rc);
            break;
        }
        shell_print(shell, "%s", data);
        if (rc < sizeof(data))
        { // reached EOF
            break;
        }
        offset += rc;
    }
    return 0;
}

static int cmd_fs_write(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 3)
    {
        shell_print(shell, "error  argc = %d", argc);
        return 0;
    }
    int rc = storage_write(argv[1], argv[2], strlen(argv[2]));
    if (rc < 0)
    {
        shell_error(shell, "FAIL: write %s: %d", argv[1], rc);
        return rc;
    }
    else
    {
        shell_print(shell, "write %s: %d", argv[1], rc);
    }
    return 0;
}
static int cmd_fs_append(const struct shell *shell, size_t argc, char **argv)
{
    struct fs_file_t file;
    char path[MAX_PATH_LEN];

    // shell_print(shell, "%s", __func__);
    if (argc < 3)
    {
        shell_print(shell, "error  argc = %d", argc);
        return 0;
    }
    struct fs_mount_t *mp = storage_mount_point_get();

    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, argv[1]);

    fs_file_t_init(&file);

    int rc = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR | FS_O_APPEND);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: open %s: %d", path, rc);
        goto out;
    }
    ssize_t len = fs_write(&file, argv[2], strlen(argv[2]));
    if (len < 0)
    {
        shell_error(shell, "FAIL: write %s: %d", path, rc);
        goto out;
    }

out:
    int ret = fs_close(&file);
    if (ret < 0)
    {
        shell_error(shell, "FAIL: close %s: %d", path, ret);
        return ret;
    }
    return 0;
}
static int cmd_fs_remove(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2)
    {
        shell_print(shell, "error  argc = %d", argc);
        return 0;
    }
    int rc = storage_remove(argv[1]);
    {
        if (rc < 0)
        {
            shell_error(shell, "FAIL: remove %s: %d", argv[1], rc);
        }
        else
        {
            shell_print(shell, "remove %s: %d", argv[1], rc);
        }
    }
    return 0;
}
static int cmd_fs_move(const struct shell *shell, size_t argc, char **argv)
{
    int rc;

    shell_print(shell, "%s", __func__);
    if (argc < 3)
    {
        shell_print(shell, "error  argc = %d", argc);
        return 0;
    }
    rc = storage_move(argv[1], argv[2]);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: move %s: %d", argv[1], rc);
    }
    else
    {
        shell_print(shell, "move %s: %d", argv[1], rc);
    }
    return 0;
}

static int cmd_fs_erase(const struct shell *shell, size_t argc, char **argv)
{

    shell_print(shell, "%s", __func__);

    struct fs_mount_t *mp = storage_mount_point_get();
    int rc = storage_flash_erase((uintptr_t)mp->storage_dev);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: erase %d", rc);
    }
    return 0;
}
static int cmd_fs_counter_set(const struct shell *shell, size_t argc, char **argv)
{
    int result;
    int rc = storage_counter_set(argv[1], atoi(argv[2]), &result);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: set counter %d", rc);
    }
    else
    {
        shell_print(shell, "set counter %d", result);
    }
    return 0;
}
static int cmd_fs_counter_get(const struct shell *shell, size_t argc, char **argv)
{
    int result;
    int rc = storage_counter_get(argv[1], &result);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: get counter %d", rc);
    }
    else
    {
        shell_print(shell, "get counter %d", result);
    }
    return 0;
}
static int cmd_fs_counter_adjust(const struct shell *shell, size_t argc, char **argv)
{
    int result;
    int rc = storage_counter_adjust(argv[1], atoi(argv[2]), &result);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: adjust counter %d", rc);
    }
    else
    {
        shell_print(shell, "adjust counter %d", result);
    }
    return 0;
}

#ifdef CONFIG_FS_TEST
static int cmd_fs_test(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "%s", __func__);
    fs_test(shell);
    return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_fs,
                               SHELL_CMD(ls, NULL, "list", cmd_fs_ls),
                               SHELL_CMD(stats, NULL, "stats", cmd_fs_stats),
                               SHELL_CMD(read, NULL, "read", cmd_fs_read),
                               SHELL_CMD(read_ascii, NULL, "read_ascii", cmd_fs_read_ascii),
                               SHELL_CMD(write, NULL, "write", cmd_fs_write),
                               SHELL_CMD(append, NULL, "append", cmd_fs_append),
                               SHELL_CMD(rm, NULL, "remove", cmd_fs_remove),
                               SHELL_CMD(mv, NULL, "mv", cmd_fs_move),
                               SHELL_CMD(erase, NULL, "erase", cmd_fs_erase),
                               SHELL_CMD(cnt_set, NULL, "counter get [file] [new val]", cmd_fs_counter_set),
                               SHELL_CMD(cnt_get, NULL, "counter get [file]", cmd_fs_counter_get),
                               SHELL_CMD(cnt_adjust, NULL, "counter adjust [file] [adjustment]", cmd_fs_counter_adjust),
#ifdef CONFIG_FS_TEST
                               SHELL_CMD(test, NULL, "test", cmd_fs_test),
#endif
                               SHELL_SUBCMD_SET_END /* Array terminated. */

);

SHELL_CMD_REGISTER(fs, &sub_fs, "file system", NULL);