#ifndef __STATUS_OBJ_H__
#define __STATUS_OBJ_H__
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>

struct axis_pos
{
    int32_t step_pos;
    int32_t tar_pos;
    int32_t last_skip;
    int32_t tot_skip;
    char *state;
};
struct axis_cfg
{
    int16_t duty;
    uint32_t per;
    int16_t i_lim;
};

struct accel_telem
{
    char *p;
    char *r;
    char *temp_c;
};

struct axis_telem
{
    int32_t b_emf;
};
struct pos
{
    struct axis_pos axis_0;
    struct axis_pos axis_1;
    int32_t ts;
};
struct cfg
{
    struct axis_cfg axis_0;
    struct axis_cfg axis_1;
};
struct telem
{
    struct axis_telem axis_0;
    struct axis_telem axis_1;
    struct accel_telem accel;
};
struct motor_fsm_state
{
    char *state;
    char *h_state;
};
typedef struct
{
    struct pos pos;
    struct cfg cfg;
    struct telem telem;
    struct motor_fsm_state motor_fsm_state;
} status_t;

status_t *heliostat_status_get(void);

/*
const char *status_obj_state_str_get(MotorFsmState_t cur_state);
const char *status_obj_home_state_str_get(HomingFsmState_t h_state);
*/

#endif // __STATUS_OBJ_H__