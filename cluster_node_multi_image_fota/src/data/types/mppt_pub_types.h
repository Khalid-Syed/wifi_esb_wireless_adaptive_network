#ifndef MPPT_PUB_TYPES_H
#define MPPT_PUB_TYPES_H

typedef struct 
{
    int32_t mppt_state;
    int32_t gate_duty;
    int32_t mv;
    int32_t ma;
    int32_t mw;
    int32_t bat_mv;
    int32_t tenth_C;
} mppt_pub_t;

#endif // MPPT_PUB_TYPES_H
