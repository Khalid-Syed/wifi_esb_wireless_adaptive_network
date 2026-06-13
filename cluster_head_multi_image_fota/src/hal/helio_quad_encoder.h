#ifndef __HELIO_QUAD_ENCODER_H__
#define __HELIO_QUAD_ENCODER_H__

#include <stdio.h>
#include <string.h>

typedef enum
{
    ENCODER_IDX_0 = 0,
    ENCODER_IDX_1 = 1
} QuadEncoderIdx_t;

int quad_encoder_init(QuadEncoderIdx_t idx);
int quad_encoder_set(QuadEncoderIdx_t idx, int degrees);
int quad_encoder_poll(QuadEncoderIdx_t encoder);
void quad_encoder_status_log(QuadEncoderIdx_t encoder);
void quad_encoder_config_log(QuadEncoderIdx_t encoder);

#endif // __HELIO_QUAD_ENCODER_H__