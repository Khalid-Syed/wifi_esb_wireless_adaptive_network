#ifndef CFG_JSON_H
#define CFG_JSON_H
#include "cfg.h"
/* module to decode cfg JSON payloads */

/**
 * @brief decode a JSON string into a cfg_t struct
 * 
 * @param json_str 
 * @param cfg_p 
 * @return int 
 */
int cfg_decode(char *json_str, cfg_t *cfg_p);
void log_cfg(cfg_t *cfg_p);

#define PAYLOAD_DECODE cfg_decode

#endif // CFG_JSON_H
