#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/data/json.h>
#include "status_obj.h"
#include "motor_constants.h"

/*
static const char *INIT_JSON = "init";
static const char *FAULTED_JSON = "fault";
static const char *IDLE_JSON = "idle";
static const char *MOVING_JSON = "moving";
static const char *HOLDING_JSON = "holding";
static const char *HOMING_JSON = "homing";
static const char *UNKNOWN_JSON = "unknown";
static const char *RETRY_JSON = "retry";
static const char *HARD_FAULT_JSON = "hard_fault";

static const char *HOMING_INIT_JSON = "init";
static const char *HOMING_LEVEL_JSON = "lev";
static const char *HOMING_LOWER_JSON = "low";
static const char *HOMING_HOME_JSON = "home";
static const char *HOMING_RECHECK_JSON = "rechk";
static const char *HOMING_FINISH_JSON = "fin";
static const char *HOMING_FAILED_JSON = "fail";
static const char *HOMING_IDLE_JSON = "idle";
*/

static status_t status_obj;

status_t *heliostat_status_get(void)
{
    // insert current timestamp before returning
    status_obj.pos.ts = k_uptime_get_32();

    return &status_obj;
}

/*
const char *status_obj_state_str_get(MotorFsmState_t cur_state)
{
    const char *json_state = NULL;

    switch (cur_state)
    {
        case MOTOR_STATE_INIT:
            json_state = INIT_JSON;
            break;
        case MOTOR_STATE_FAULTED:
            json_state = FAULTED_JSON;
            break;
        case MOTOR_STATE_IDLE:
            json_state = IDLE_JSON;
            break;
        case MOTOR_STATE_MOVING:
            json_state = MOVING_JSON;
            break;
        case MOTOR_STATE_HOMING:
            json_state = HOMING_JSON;
            break;
        case MOTOR_STATE_HOLDING:
            json_state = HOLDING_JSON;
            break;
        case MOTOR_STATE_RETRY:
            json_state = RETRY_JSON;
            break;
        case MOTOR_STATE_HARD_FAULT:
            json_state = HARD_FAULT_JSON;
            break;
        default:
            json_state = UNKNOWN_JSON;
            break;
    }

    return json_state;
}

const char *status_obj_home_state_str_get(HomingFsmState_t h_state)
{
    const char *home_state = NULL;

    switch (h_state)
    {
        case HOMING_STATE_INIT:
            home_state = HOMING_INIT_JSON;
            break;
        case HOMING_STATE_LEVEL:
            home_state = HOMING_LEVEL_JSON;
            break;
        case HOMING_STATE_LOWER:
            home_state = HOMING_LOWER_JSON;
            break;
        case HOMING_STATE_HOME:
            home_state = HOMING_HOME_JSON;
            break;
        case HOMING_STATE_RECHECK:
            home_state = HOMING_RECHECK_JSON;
            break;
        case HOMING_STATE_FINISH:
            home_state = HOMING_FINISH_JSON;
            break;
        case HOMING_STATE_FAILED:
            home_state = HOMING_FAILED_JSON;
            break;
        case HOMING_STATE_IDLE:
            home_state = HOMING_IDLE_JSON;
            break;
        default:
            home_state = UNKNOWN_JSON;
            break;
        }

    return home_state;
}
*/