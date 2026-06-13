#ifndef SYS_PUB_JSON_H
#define SYS_PUB_JSON_H
#include "sys_pub_types.h" // example of a data object - make your own

#define SYS_PUB_PAYLOAD_STR_SIZE_MAX 1024
#define PAYLOAD_ENCODE sys_pub_json_encode
int sys_pub_json_encode(sys_info_t *sys_info, char *buf, int buf_len);

#endif // SYS_PUB_JSON_H
