#ifndef MPPT_PUB_JSON_H
#define MPPT_PUB_JSON_H
#include "mppt_pub_types.h"

#define MPPT_PUB_PAYLOAD_STR_SIZE_MAX 1024
#define PAYLOAD_ENCODE mppt_pub_json_encode
int mppt_pub_json_encode(mppt_pub_t *mppt_pub, char *buf, int buf_len);

#endif // MPPT_PUB_JSON_H
