
#include "storage.h"
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#define MODULE storage_test

#warning "building with storage_test.c"

LOG_MODULE_REGISTER(MODULE);

static int littlefs_increase_infile_value(const struct shell *shell, char *fname);
static void incr_pattern(uint8_t *p, uint16_t size, uint8_t inc);
static void init_pattern(uint8_t *p, uint16_t size);
static int littlefs_binary_file_adj(const struct shell *shell, char *fname);
// static int littlefs_mount(const struct shell *shell, struct fs_mount_t *mp);
extern int lsdir(const struct shell *shell, const char *path);

void fs_test(const struct shell *shell)
{
    char fname1[MAX_PATH_LEN];
    char fname2[MAX_PATH_LEN];
    struct fs_statvfs sbuf;
    int rc;

    shell_print(shell, "Test r/w files on littlefs");
    struct fs_mount_t *mp = storage_mount_point_get();

    // mount if not auto-mount configured
#if !DT_NODE_EXISTS(PARTITION_NODE) || \
    !(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
    rc = littlefs_mount(shell, mp);
    if (rc < 0)
    {
        return;
    }
#else /* Do not unmount if auto-mount has been enabled */
    shell_print(shell, "%s automounted - do not mount\n", mp->mnt_point);
#endif

    snprintf(fname1, sizeof(fname1), "%s/boot_count", mp->mnt_point);
    snprintf(fname2, sizeof(fname2), "%s/pattern.bin", mp->mnt_point);

    rc = fs_statvfs(mp->mnt_point, &sbuf);
    if (rc < 0)
    {
        shell_print(shell, "FAIL: statvfs: %d", rc);
        goto out;
    }

    shell_print(shell, "%s: bsize = %lu ; frsize = %lu ;"
                       " blocks = %lu ; bfree = %lu",
                mp->mnt_point,
                sbuf.f_bsize, sbuf.f_frsize,
                sbuf.f_blocks, sbuf.f_bfree);

    char list[1024];
    list[0] = '\0';
    rc = storage_list(list);
    if (rc < 0)
    {
        shell_print(shell, "FAIL: lsdir %s: %d", mp->mnt_point, rc);
        goto out;
    }
    else
    {
        shell_print(shell, "lsdir %s: %d", mp->mnt_point, rc);
        shell_print(shell, "%s", list);
    }

    rc = littlefs_increase_infile_value(shell, fname1);
    if (rc)
    {
        goto out;
    }

    rc = littlefs_binary_file_adj(shell, fname2);
    if (rc)
    {
        goto out;
    }

out:
    // unmount if not auto-mount configured
#if !DT_NODE_EXISTS(PARTITION_NODE) || \
    !(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
    rc = fs_unmount(mp);
    shell_print(shell, "%s unmount: %d", mp->mnt_point, rc);
#else /* Do not unmount if auto-mount has been enabled */
    shell_print(shell, "%s automounted - do not unmount\n", mp->mnt_point);
#endif
}

/***********************
 *  private functions
 **********************/
static int littlefs_increase_infile_value(const struct shell *shell, char *fname)
{
    uint8_t boot_count = 0;
    struct fs_file_t file;
    int rc, ret;

    fs_file_t_init(&file);
    rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: open %s: %d", fname, rc);
        return rc;
    }

    rc = fs_read(&file, &boot_count, sizeof(boot_count));
    if (rc < 0)
    {
        shell_error(shell, "FAIL: read %s: [rd:%d]", fname, rc);
        goto out;
    }
    shell_print(shell, "%s read count:%u (bytes: %d)", fname, boot_count, rc);

    rc = fs_seek(&file, 0, FS_SEEK_SET);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: seek %s: %d", fname, rc);
        goto out;
    }

    boot_count += 1;
    rc = fs_write(&file, &boot_count, sizeof(boot_count));
    if (rc < 0)
    {
        shell_error(shell, "FAIL: write %s: %d", fname, rc);
        goto out;
    }

    shell_print(shell, "%s write new boot count %u: [wr:%d]", fname,
                boot_count, rc);

out:
    ret = fs_close(&file);
    if (ret < 0)
    {
        shell_error(shell, "FAIL: close %s: %d", fname, ret);
        return ret;
    }

    return (rc < 0 ? rc : 0);
}

static void incr_pattern(uint8_t *p, uint16_t size, uint8_t inc)
{
    uint8_t fill = 0x55;

    if (p[0] % 2 == 0)
    {
        fill = 0xAA;
    }

    for (int i = 0; i < (size - 1); i++)
    {
        if (i % 8 == 0)
        {
            p[i] += inc;
        }
        else
        {
            p[i] = fill;
        }
    }

    p[size - 1] += inc;
}

static void init_pattern(uint8_t *p, uint16_t size)
{
    uint8_t v = 0x1;

    memset(p, 0x55, size);

    for (int i = 0; i < size; i += 8)
    {
        p[i] = v++;
    }

    p[size - 1] = 0xAA;
}

static int littlefs_binary_file_adj(const struct shell *shell, char *fname)
{
    struct fs_dirent dirent;
    struct fs_file_t file;
    int rc, ret;
    uint8_t file_test_pattern[TEST_FILE_SIZE];

    /*
     * Uncomment below line to force re-creation of the test pattern
     * file on the littlefs FS.
     */
    /* fs_unlink(fname); */
    fs_file_t_init(&file);

    rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: open %s: %d", fname, rc);
        return rc;
    }

    rc = fs_stat(fname, &dirent);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: stat %s: %d", fname, rc);
        goto out;
    }

    /* Check if the file exists - if not just write the pattern */
    if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0)
    {
        shell_print(shell, "Test file: %s not found, create one!",
                    fname);
        init_pattern(file_test_pattern, sizeof(file_test_pattern));
    }
    else
    {
        rc = fs_read(&file, file_test_pattern,
                     sizeof(file_test_pattern));
        if (rc < 0)
        {
            shell_error(shell, "FAIL: read %s: [rd:%d]",
                        fname, rc);
            goto out;
        }
        incr_pattern(file_test_pattern, sizeof(file_test_pattern), 0x1);
    }

    shell_print(shell, "------ FILE: %s ------", fname);
    shell_hexdump(shell, file_test_pattern, sizeof(file_test_pattern));

    rc = fs_seek(&file, 0, FS_SEEK_SET);
    if (rc < 0)
    {
        shell_error(shell, "FAIL: seek %s: %d", fname, rc);
        goto out;
    }

    rc = fs_write(&file, file_test_pattern, sizeof(file_test_pattern));
    if (rc < 0)
    {
        shell_error(shell, "FAIL: write %s: %d", fname, rc);
    }

out:
    ret = fs_close(&file);
    if (ret < 0)
    {
        shell_error(shell, "FAIL: close %s: %d", fname, ret);
        return ret;
    }

    return (rc < 0 ? rc : 0);
}

// we are auto-mounting, but keep the code for manual mounting reference
#if !DT_NODE_EXISTS(PARTITION_NODE) || \
    !(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)

static int littlefs_mount(const struct shell *shell, struct fs_mount_t *mp)
{
    int rc;

    rc = storage_flash_erase((uintptr_t)mp->storage_dev);
    if (rc < 0)
    {
        return rc;
    }

    /* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) || \
    !(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
    rc = fs_mount(mp);
    if (rc < 0)
    {
        shell_print(shell, "FAIL: mount id %" PRIuPTR " at %s: %d\n",
                    (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
        return rc;
    }
    shell_print(shell, "%s mount: %d\n", mp->mnt_point, rc);
#else
    shell_print(shell, "%s automounted\n", mp->mnt_point);
#endif

    return 0;
}
#endif