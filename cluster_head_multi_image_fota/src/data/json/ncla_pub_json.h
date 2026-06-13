#ifndef NCLA_PUB_JSON_H
#define NCLA_PUB_JSON_H
#include "ncla_pub_types.h"

#define NCLA_PUB_PAYLOAD_STR_SIZE_MAX 1024
#define PAYLOAD_ENCODE ncla_pub_json_encode
int ncla_pub_json_encode(ncla_pub_t *ncla_pub, char *buf, int buf_len);

#endif // NCLA_PUB_JSON_H
