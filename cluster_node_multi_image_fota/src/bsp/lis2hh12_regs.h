#ifndef __LIS2HH12_REGS_H__
#define __LIS2HH12_REGS_H__

#define LIS2HH12_WHO_AM_I   0x0FU

//###############################
// Register Definitions
//#########################
#define TEMP_L          0x0B
#define TEMP_H          0x0C
#define WHO_AM_I        0x0F
#define ACT_THS         0x1E
#define ACT_DUR         0x1F
#define CTRL1           0x20
#define CTRL2           0x21
#define CTRL3           0x22
#define CTRL4           0x23
#define CTRL5           0x24
#define CTRL6           0x25
#define CTRL7           0x26
#define STATUS          0x27
#define DATA_FIRST_BYTE 0x28
#define FIFO_CTRL       0x2E
#define FIFO_SRC        0x2F
#define IG_CFG1         0x30
#define IG_SRC1         0x31
#define IG_THS_X1       0x32
#define IG_THS_Y1       0x33       
#define IG_THS_Z1       0x34
#define IG_DUR1         0x35
#define IG_CFG2         0x36
#define IG_SRC2         0x37
#define IG_THS2         0x38
#define IG_DUR2         0x39
#define XL_REFERENCE    0x3A
#define XH_REFERENCE    0x3B
#define YL_REFERENCE    0x3C
#define YH_REFERENCE    0x3D
#define ZL_REFERENCE    0x3E
#define ZH_REFERENCE    0x3F

#endif // __LIS2HH12_REGS_H__