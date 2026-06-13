#ifndef __STORAGE_H__
#define __STORAGE_H__
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

/* Matches LFS_NAME_MAX */
#define MAX_PATH_LEN 255
#define TEST_FILE_SIZE 547
#define PARTITION_NODE DT_NODELABEL(lfs1)
/**
 * @brief get mount point for file system in serial flash
 *
 * @return struct fs_mount_t*
 */
struct fs_mount_t *storage_mount_point_get(void);
/**
 * @brief erase file system - DANGER
 *
 * @param id
 * @return int
 */
int storage_flash_erase(unsigned int id);
/**
 * @brief write data to file
 *
 * @param file_name
 * @param data
 * @param len
 * @return int
 */
int storage_write(const char *file_name, const char *data, size_t len);
/**
 * @brief read data from file
 *
 * @param file_name
 * @param offset
 * @param data
 * @param max_len
 * @return int
 */
int storage_read(const char *file_name, off_t offset, char *data, size_t max_len);
/**
 * @brief remove file
 *
 * @param file_name
 * @return int
 */
int storage_remove(const char *file_name);
/**
 * @brief move file
 *
 * @param file_name_from
 * @param file_name_to
 * @return int
 */
int storage_move(const char *file_name_from, const char *file_name_to);
/**
 * @brief list files in file system - list will be ascii string of file names separated by \n
 *
 * @param list
 * @param max_len
 * @return int
 */
int storage_list(char *list, int max_len);

#endif