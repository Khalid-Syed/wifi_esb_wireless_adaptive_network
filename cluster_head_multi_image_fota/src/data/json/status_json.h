#ifndef STATUS_JSON_H
#define STATUS_JSON_H
#include "status_obj.h"
/*
{
2    "status": {
3        "pos": {
4            "axis_0": {
5                "step_pos": 0,
6                "tar_pos": 0,
8                "state": "init"
9            },
10            "axis_1": {
11                "step_pos": 0,
12                "tar_pos": 0,
14                "state": "init"
15            },
16            "ts": 0
17        },
18        "cfg": {
19            "axis_0": {
20                "duty": 0,
21                "per": 0,
22                "i_lim": 0
23            },
24            "axis_1": {
25                "duty": 0,
26                "per": 0,
27                "i_lim": 0
28            }
29        },
30        "telem": {
31            "axis_0": {
33                "b_emf": 0
34            },
35            "axis_1": {
37                "b_emf": 0
38            },
              "accel": {
                "p": "0.0",
                "r": "0.0"
              }
39        },
40        "motor_fsm_state": {
41            "state": "init",
              "h_state": "idle"
42        }
43    }
44}
*/

#define STATUS_PAYLOAD_STR_SIZE_MAX 1024
#define PAYLOAD_ENCODE status_json_encode
int status_json_encode(status_t *status, char *buf, int buf_len);

#endif // STATUS_JSON_H
