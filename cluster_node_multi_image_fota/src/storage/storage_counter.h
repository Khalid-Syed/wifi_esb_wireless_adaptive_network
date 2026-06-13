#ifndef __STORAGE_COUNTER_H__
#define __STORAGE_COUNTER_H__
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "storage.h"

/*
    This module is used to store and retrieve a counter from a file.
    The counter is stored as an integer in the file.
    The file is created if it does not exist.
*/

/**
 * @brief set counter in file
 *
 * @param fname
 * @param in
 * @param pointer to result from file
 * @return int return 0 if success, otherwise return < 0
 */
int storage_counter_set(char *fname, int in, int *out);
/**
 * @brief get counter from file
 *
 * @param fname
 * @param out pointer to result from file
 * @return int return 0 if success, otherwise return < 0
 */
int storage_counter_get(char *fname, int *out);
/**
 * @brief adjust counter in file
 *
 * @param fname
 * @param in  integer to adjust +/-
 * @param out pointer to result from file
 * @return int return 0 if success, otherwise return < 0
 */
int storage_counter_adjust(char *fname, int in, int *out);

#endif