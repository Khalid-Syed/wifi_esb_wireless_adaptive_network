#ifndef __PUB_TABLE_H__
#define __PUB_TABLE_H__
#include <zephyr/kernel.h>

typedef struct
{
    const char *topic;
    struct k_work_delayable *dwork;
    int (*init)(void); // optional init function
    int (*pub)(void);  // callback to publish data
    int initial_interval_ms;
    int intial_interval_jitter_ms;
    int interval_ms;
    int interval_jitter_ms;
} pub_t;

extern pub_t pub_table[];
int pub_table_size(void);

#endif // __PUB_TABLE_H__