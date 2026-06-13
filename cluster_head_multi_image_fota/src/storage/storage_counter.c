
#include "storage.h"
#define MODULE storage_counter

LOG_MODULE_REGISTER(MODULE, CONFIG_FS_LOG_LEVEL);

typedef enum
{
    STORAGE_COUNTER_SET,
    STORAGE_COUNTER_GET,
    STORAGE_COUNTER_ADJUST
} action_t;

static int storage_manipulate_counter(char *fname, int in, int *out, action_t action);

/** public functions **/
int storage_counter_set(char *fname, int in, int *out)
{
    return storage_manipulate_counter(fname, in, out, STORAGE_COUNTER_SET);
}
int storage_counter_get(char *fname, int *out)
{
    return storage_manipulate_counter(fname, 0, out, STORAGE_COUNTER_GET);
}
int storage_counter_adjust(char *fname, int in, int *out)
{
    return storage_manipulate_counter(fname, in, out, STORAGE_COUNTER_ADJUST);
}

/** private functions **/
static int storage_manipulate_counter(char *fname, int in, int *out, action_t action)
{
    int32_t counter = 0;
    char path[MAX_PATH_LEN];
    struct fs_file_t file;
    struct fs_mount_t *mp = storage_mount_point_get();
    bool empty_file = false;
    int ret, rc;

    snprintf(path, sizeof(path), "%s/%s", mp->mnt_point, fname);

    fs_file_t_init(&file);
    LOG_DBG("%s path = %s", __func__, path);
    rc = fs_open(&file, path, FS_O_CREATE | FS_O_RDWR);
    if (rc < 0)
    {
        LOG_ERR("FAIL: open %s: %d", path, rc);
        return rc;
    }

    rc = fs_read(&file, &counter, sizeof(counter));
    if (rc < 0)
    {
        LOG_ERR("FAIL: read %s: [rd:%d]", fname, rc);
        goto fini;
    }
    LOG_DBG("%s read count:%u (bytes: %d)", fname, counter, rc);
    if (rc == 0)
    {
        LOG_DBG("file is empty, setting counter to 0");
        counter = 0;
        empty_file = true;
    }

    switch (action)
    {
    case STORAGE_COUNTER_SET:
        counter = in;
        *out = counter;
        break;
    case STORAGE_COUNTER_GET:
        *out = counter;
        if (!empty_file)
        {
            goto fini; // no need to write back
        }
        break;
    case STORAGE_COUNTER_ADJUST:
        counter += in;
        *out = counter;
        break;
    }

    rc = fs_seek(&file, 0, FS_SEEK_SET);
    if (rc < 0)
    {
        LOG_ERR("FAIL: seek %s: %d", fname, rc);
        goto fini;
    }
    rc = fs_write(&file, &counter, sizeof(counter));
    if (rc < 0)
    {
        LOG_ERR("FAIL: write %s: %d", fname, rc);
    }

fini:
    ret = fs_close(&file);
    if (ret < 0)
    {
        LOG_ERR("FAIL: close %s: %d", fname, ret);
        return ret;
    }

    LOG_DBG("%s: %s in = %d, out = %d", __func__, fname, in, *out);
    return (rc < 0 ? rc : 0);
}
