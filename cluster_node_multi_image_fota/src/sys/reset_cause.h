#ifndef __RESET_CAUSE_H__
#define __RESET_CAUSE_H__

#define RESET_CAUSE_STR_MAX_LEN 128

/**
 * @brief Get reset cause string
 *
 * @return char*
 */
char *reset_cause_get(void);

#endif