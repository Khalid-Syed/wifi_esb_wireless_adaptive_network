#ifndef TEMPLATE_PUB_JSON_H
#define TEMPLATE_PUB_JSON_H
#include "template_pub_types.h" // example of a data object - make your own

#define TEMPLATE_PUB_PAYLOAD_STR_SIZE_MAX 1024
#define PAYLOAD_ENCODE template_pub_json_encode
int template_pub_json_encode(template_pub_t *template_pub, char *buf, int buf_len);

#endif // TEMPLATE_PUB_JSON_H
