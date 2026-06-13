#ifndef __ASSERT_POST_ACTION_H__
#define __ASSERT_POST_ACTION_H__

#define ASSERT_STR_LEN_MAX 256
#define ASSERT_FILE_NAME "assert.json"

/**
 * @brief assert post action - called by assert compiler macro
 *
 * @param file
 * @param line
 */
void assert_post_action(const char *file, unsigned int line);

/**
 * @brief read assert file
 *
 * @param assert
 * @param len
 * @return int
 */
int assert_json_read(char *assert, int len);
/**
 * @brief write assert message to file
 *
 * @param assert_msg
 * @return int
 */
int assert_string_to_file(const char *assert_msg);

#endif