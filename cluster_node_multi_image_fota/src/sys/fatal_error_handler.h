#ifndef __FATAL_ERROR_H__
#define __FATAL_ERROR_H__

#define FATAL_ERROR_STR_LEN_MAX 256
#define FATAL_ERROR_FILE_NAME "fatal.json"

/**
 * @brief fatal error handler
 *
 * @param reason
 * @param esf
 */
void k_sys_fatal_error_handler(unsigned int reason,
                               const z_arch_esf_t *esf);
/**
 * @brief save fatal error string to file
 *
 * @param fatal_error_msg
 * @return int
 */
int fatal_error_to_file(const char *fatal_error_msg);

/**
 * @brief read fatal error file
 *
 * @param len
 * @return int
 */
int fatal_error_json_read(char *fatal_error, int len);

#endif