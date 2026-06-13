#ifndef __NULL_PARAM_CHECK_H__
#define __NULL_PARAM_CHECK_H__
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

/**@brief Check if the input pointer is NULL, if so it returns -EINVAL. */
#define NULL_PARAM_CHECK(param) \
    do                          \
    {                           \
        if ((param) == NULL)    \
        {                       \
            return -EINVAL;     \
        }                       \
    } while (0)

#define NULL_PARAM_CHECK_VOID(param) \
    do                               \
    {                                \
        if ((param) == NULL)         \
        {                            \
            return;                  \
        }                            \
    } while (0)

#endif //__NULL_PARAM_CHECK_H__