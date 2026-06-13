
#include "storage.h"

#define MODULE storage

LOG_MODULE_REGISTER(MODULE, CONFIG_STORAGE_LOG_LEVEL);

/***
 * THIS MODULE WRAPS THE LITTLEFS FILE SYSTEM
 */

#if DT_NODE_EXISTS(PARTITION_NODE)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else  /* PARTITION_NODE */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
    .mnt_point = "/lfs",
};
#endif /* PARTITION_NODE */

struct fs_mount_t *mp =
#if DT_NODE_EXISTS(PARTITION_NODE)
    &FS_FSTAB_ENTRY(PARTITION_NODE)
#else
    &lfs_storage_mnt
#endif
    ;
/**
 * @brief get the mount point for the file system
 *
 * @return struct fs_mount_t*
 */
struct fs_mount_t *storage_mount_point_get(void)
{
    return mp;
}
/**
 * @brief write data to file
 *
 * @param id
 * @return int
 */
int storage_flash_erase(unsigned int id)
{
    const struct flash_area *pfa;
    int rc;

    rc = flash_area_open(id, &pfa);
    if (rc < 0)
    {
        LOG_ERR("FAIL: unable to find flash area %u: %d",
                id, rc);
        return rc;
    }

    LOG_INF("Area %u at 0x%x on %s for %u bytes",
            id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
            (unsigned int)pfa->fa_size);

    /* Optional wipe flash contents */
    if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE))
    {
        rc = flash_area_erase(pfa, 0, pfa->fa_size);
        LOG_ERR("Erasing flash area ... %d", rc);
    }

    flash_area_close(pfa);
    return rc;
}
/**
 * @brief read data from file
 *
 * @param file_name
 * @param offset
 * @param data
 * @param max_len
 * @return int
 */
int storage_read(const char *file_name, off_t offset, char *data, size_t max_len)
{
    struct fs_file_t file;
    char path[MAX_PATH_LEN];
    LOG_DBG("%s %s offset %d max len %d", __func__, file_name, (int)offset, max_len);
    if (file_name == NULL || data == NULL || max_len == 0)
    {
        LOG_ERR("FAIL: invalid parameters");
        return -EINVAL;
    }
    // calculate path
    struct fs_mount_t *mp = storage_mount_point_get();
    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, file_name);
    LOG_DBG("%s path = %s", __func__, path);

    // open file
    fs_file_t_init(&file);
    int rc = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR);
    if (rc < 0)
    {
        LOG_ERR("FAIL: open %s: %d", path, rc);
        goto out;
    }
    // seek to offset in file
    rc = fs_seek(&file, offset, FS_SEEK_SET);
    if (rc < 0)
    {
        LOG_ERR("FAIL: seek %s: %d", path, rc);
        goto out;
    }
    LOG_DBG("%s read %s %d max bytes to %p", __func__, path, max_len, data);
    // read data from file
    rc = fs_read(&file, data, max_len);
    LOG_DBG("rc = %d", rc);
    if (rc < 0)
    {
        LOG_ERR("FAIL: read %s: [rd:%d]", path, rc);
        goto out;
    }

out:
    fs_close(&file); // silent close
    return rc;
}
/**
 * @brief write data to file
 *
 * @param file_name
 * @param data
 * @param len
 * @return int
 */
int storage_write(const char *file_name, const char *data, size_t len)
{
    struct fs_file_t file;
    char path[MAX_PATH_LEN];
    struct fs_mount_t *mp = storage_mount_point_get();

    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, file_name);

    fs_file_t_init(&file);

    int rc = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR);
    if (rc < 0)
    {
        LOG_ERR("FAIL: open %s: %d", path, rc);
        goto out;
    }
    len = fs_write(&file, data, len);
    if (len < 0)
    {
        LOG_ERR("FAIL: write %s: %d", path, rc);
        goto out;
    }

out:
    fs_close(&file); // silent close

    return rc;
}
/**
 * @brief remove file
 *
 * @param file_name
 * @return int
 */
int storage_remove(const char *file_name)
{
    char path[MAX_PATH_LEN];
    struct fs_mount_t *mp = storage_mount_point_get();

    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, file_name);

    int rc = fs_unlink(path);
    if (rc < 0)
    {
        if (rc == -EINVAL)
        {
            LOG_DBG("FAIL: bad file name %s: %d", path, rc);
            goto out;
        }
        else if (rc == -EROFS)
        {
            LOG_ERR("FAIL: file is read-only %s: %d", path, rc);
            goto out;
        }
        else if (rc == -ENOTSUP)
        {
            LOG_ERR("FAIL: not implemented by underlying file system driver %s: %d", path, rc);
            goto out;
        }
        LOG_ERR("FAIL: unlink %s: %d", path, rc);
        goto out;
    }
    else
    {
        LOG_DBG("unlinked %s: %d", path, rc);
    }
out:
    return rc;
}
/**
 * @brief move file
 *
 * @param from
 * @param to
 * @return int
 */
int storage_move(const char *from, const char *to)
{
    char from_path[MAX_PATH_LEN];
    char to_path[MAX_PATH_LEN];

    struct fs_mount_t *mp = storage_mount_point_get();

    snprintf(from_path, sizeof(from_path), "%s/%s", mp->mnt_point, from);
    snprintf(to_path, sizeof(to_path), "%s/%s", mp->mnt_point, to);

    int rc = fs_rename(from_path, to_path);
    if (rc < 0)
    {
        if (rc == -EINVAL)
        {
            LOG_ERR("FAIL: bad file name %d", rc);
            goto out;
        }
        else if (rc == -EROFS)
        {
            LOG_ERR("FAIL: file is read-only %d", rc);
            goto out;
        }
        else if (rc == -ENOTSUP)
        {
            LOG_ERR("FAIL: not implemented by underlying file system driver %d", rc);
            goto out;
        }
        LOG_ERR("FAIL: move rc = %d", rc);
        goto out;
    }
    else
    {
        LOG_DBG("move %s -> %s", from_path, to_path);
    }
out:
    return 0;
}
/**
 * @brief list files in directory - not recursive
 *
 * @param list
 * @param max_len
 * @return int
 */
// int storage_lsdir(const char *path, char *list) int
int storage_list(char *list, int max_len)
{
    int res;
    struct fs_dir_t dirp;
    static struct fs_dirent entry;

    struct fs_mount_t *mp = storage_mount_point_get();

    const char *abs_path = mp->mnt_point; // + path  - not worrying about appending path for now, just read root dir

    fs_dir_t_init(&dirp);

    /* Verify fs_opendir() */
    res = fs_opendir(&dirp, abs_path);
    if (res)
    {
        LOG_ERR("Error opening dir %s [%d]", abs_path, res);
        return res;
    }

    int i = 0;
    LOG_DBG("\nListing dir %s ...", abs_path);
    for (;;)
    {
        /* Verify fs_readdir() */
        res = fs_readdir(&dirp, &entry);

        /* entry.name[0] == 0 means end-of-dir */
        if (res || entry.name[0] == 0)
        {
            if (res < 0)
            {
                LOG_ERR("Error reading dir [%d]", res);
            }
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR)
        {
            LOG_DBG("[DIR ] %s", entry.name);
        }
        else
        {
            LOG_DBG("[FILE] %s (size = %zu)",
                    entry.name, entry.size);
        }
        if (i + strlen(entry.name) + 1 > max_len)
        {
            LOG_ERR("List buffer too small");
            break;
        }
        else
        {
            memcpy(&list[i], entry.name, strlen(entry.name));
            memcpy(&list[i + strlen(entry.name)], "\n", 1); // new line delimiter
            i += strlen(entry.name) + 1;
        }
    }

    /* Verify fs_closedir() */
    fs_closedir(&dirp);

    // null terminate list
    memcpy(&list[i + strlen(entry.name)], "\0", 1);
    i += 1;

    return i;
}