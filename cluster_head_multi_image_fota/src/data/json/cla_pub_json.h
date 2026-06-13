#ifndef CLA_PUB_JSON_H
#define CLA_PUB_JSON_H
#include "cla_pub_types.h"

#define CLA_PUB_PAYLOAD_STR_SIZE_MAX 1024
#define PAYLOAD_ENCODE cla_pub_json_encode
int cla_pub_json_encode(cla_pub_t *cla_pub, char *buf, int buf_len);

#endif // CLA_PUB_JSON_H
