// I2Cdev library collection - MPU6050 I2C device class
// Based on InvenSense MPU-6050 register map document rev. 2.0, 5/19/2011 (RM-MPU-6000A-00)
// 10/3/2011 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg/i2cdevlib

/* ============================================
I2Cdev device library code is placed under the MIT license
Copyright (c) 2011 Jeff Rowberg
Adapted to Crazyflie FW by Bitcraze

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

#ifndef _MPU6050_H_
#define _MPU6050_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "i2cdev.h"
#include "qpc.h"

enum {
    MPU6050_TEST_SIG = QS_USER1,
};

#define MPU6050_ADDRESS_AD0_LOW     0x68 // address pin low (GND), default for InvenSense evaluation board
#define MPU6050_ADDRESS_AD0_HIGH    0x69 // address pin high (VCC)
#define MPU6050_DEFAULT_ADDRESS     MPU6050_ADDRESS_AD0_LOW
#define MPU6050_SELF_TEST_DELAY_MS  10

//Product ID Description for MPU6050:  | High 4 bits  | Low 4 bits        |
//                                     | Product Name | Product Revision  |
#define MPU6050_REV_C4_ES     0x14  //        0001           0100
#define MPU6050_REV_C5_ES     0x15  //        0001           0101
#define MPU6050_REV_D6_ES     0x16  //        0001           0110
#define MPU6050_REV_D7_ES     0x17  //        0001           0111
#define MPU6050_REV_D8_ES     0x18  //        0001           1000
#define MPU6050_REV_C4        0x54  //        0101           0100
#define MPU6050_REV_C5        0x55  //        0101           0101
#define MPU6050_REV_D6        0x56  //        0101           0110
#define MPU6050_REV_D7        0x57  //        0101           0111
#define MPU6050_REV_D8        0x58  //        0101           1000
#define MPU6050_REV_D9        0x59  //        0101           1001

#define MPU6050_RA_XG_OFFS_TC       0x00 //[7] PWR_MODE, [6:1] XG_OFFS_TC, [0] OTP_BNK_VLD
#define MPU6050_RA_YG_OFFS_TC       0x01 //[7] PWR_MODE, [6:1] YG_OFFS_TC, [0] OTP_BNK_VLD
#define MPU6050_RA_ZG_OFFS_TC       0x02 //[7] PWR_MODE, [6:1] ZG_OFFS_TC, [0] OTP_BNK_VLD
#define MPU6050_RA_X_FINE_GAIN      0x03 //[7:0] X_FINE_GAIN
#define MPU6050_RA_Y_FINE_GAIN      0x04 //[7:0] Y_FINE_GAIN
#define MPU6050_RA_Z_FINE_GAIN      0x05 //[7:0] Z_FINE_GAIN
#define MPU6050_RA_XA_OFFS_H        0x06 //[15:0] XA_OFFS
#define MPU6050_RA_XA_OFFS_L_TC     0x07
#define MPU6050_RA_YA_OFFS_H        0x08 //[15:0] YA_OFFS
#define MPU6050_RA_YA_OFFS_L_TC     0x09
#define MPU6050_RA_ZA_OFFS_H        0x0A //[15:0] ZA_OFFS
#define MPU6050_RA_ZA_OFFS_L_TC     0x0B
#define MPU6050_RA_PRODUCT_ID       0x0C
#define MPU6050_RA_XG_OFFS_USRH     0x13 //[15:0] XG_OFFS_USR
#define MPU6050_RA_XG_OFFS_USRL     0x14
#define MPU6050_RA_YG_OFFS_USRH     0x15 //[15:0] YG_OFFS_USR
#define MPU6050_RA_YG_OFFS_USRL     0x16
#define MPU6050_RA_ZG_OFFS_USRH     0x17 //[15:0] ZG_OFFS_USR
#define MPU6050_RA_ZG_OFFS_USRL     0x18
#define MPU6050_RA_SMPLRT_DIV       0x19
#define MPU6050_RA_CONFIG           0x1A
#define MPU6050_RA_GYRO_CONFIG      0x1B
#define MPU6050_RA_ACCEL_CONFIG     0x1C
#define MPU6050_RA_FF_THR           0x1D
#define MPU6050_RA_FF_DUR           0x1E
#define MPU6050_RA_MOT_THR          0x1F
#define MPU6050_RA_MOT_DUR          0x20
#define MPU6050_RA_ZRMOT_THR        0x21
#define MPU6050_RA_ZRMOT_DUR        0x22
#define MPU6050_RA_FIFO_EN          0x23
#define MPU6050_RA_I2C_MST_CTRL     0x24
#define MPU6050_RA_I2C_SLV0_ADDR    0x25
#define MPU6050_RA_I2C_SLV0_REG     0x26
#define MPU6050_RA_I2C_SLV0_CTRL    0x27
#define MPU6050_RA_I2C_SLV1_ADDR    0x28
#define MPU6050_RA_I2C_SLV1_REG     0x29
#define MPU6050_RA_I2C_SLV1_CTRL    0x2A
#define MPU6050_RA_I2C_SLV2_ADDR    0x2B
#define MPU6050_RA_I2C_SLV2_REG     0x2C
#define MPU6050_RA_I2C_SLV2_CTRL    0x2D
#define MPU6050_RA_I2C_SLV3_ADDR    0x2E
#define MPU6050_RA_I2C_SLV3_REG     0x2F
#define MPU6050_RA_I2C_SLV3_CTRL    0x30
#define MPU6050_RA_I2C_SLV4_ADDR    0x31
#define MPU6050_RA_I2C_SLV4_REG     0x32
#define MPU6050_RA_I2C_SLV4_DO      0x33
#define MPU6050_RA_I2C_SLV4_CTRL    0x34
#define MPU6050_RA_I2C_SLV4_DI      0x35
#define MPU6050_RA_I2C_MST_STATUS   0x36
#define MPU6050_RA_INT_PIN_CFG      0x37
#define MPU6050_RA_INT_ENABLE       0x38
#define MPU6050_RA_DMP_INT_STATUS   0x39
#define MPU6050_RA_INT_STATUS       0x3A
#define MPU6050_RA_ACCEL_XOUT_H     0x3B
#define MPU6050_RA_ACCEL_XOUT_L     0x3C
#define MPU6050_RA_ACCEL_YOUT_H     0x3D
#define MPU6050_RA_ACCEL_YOUT_L     0x3E
#define MPU6050_RA_ACCEL_ZOUT_H     0x3F
#define MPU6050_RA_ACCEL_ZOUT_L     0x40
#define MPU6050_RA_TEMP_OUT_H       0x41
#define MPU6050_RA_TEMP_OUT_L       0x42
#define MPU6050_RA_GYRO_XOUT_H      0x43
#define MPU6050_RA_GYRO_XOUT_L      0x44
#define MPU6050_RA_GYRO_YOUT_H      0x45
#define MPU6050_RA_GYRO_YOUT_L      0x46
#define MPU6050_RA_GYRO_ZOUT_H      0x47
#define MPU6050_RA_GYRO_ZOUT_L      0x48
#define MPU6050_RA_EXT_SENS_DATA_00 0x49
#define MPU6050_RA_EXT_SENS_DATA_01 0x4A
#define MPU6050_RA_EXT_SENS_DATA_02 0x4B
#define MPU6050_RA_EXT_SENS_DATA_03 0x4C
#define MPU6050_RA_EXT_SENS_DATA_04 0x4D
#define MPU6050_RA_EXT_SENS_DATA_05 0x4E
#define MPU6050_RA_EXT_SENS_DATA_06 0x4F
#define MPU6050_RA_EXT_SENS_DATA_07 0x50
#define MPU6050_RA_EXT_SENS_DATA_08 0x51
#define MPU6050_RA_EXT_SENS_DATA_09 0x52
#define MPU6050_RA_EXT_SENS_DATA_10 0x53
#define MPU6050_RA_EXT_SENS_DATA_11 0x54
#define MPU6050_RA_EXT_SENS_DATA_12 0x55
#define MPU6050_RA_EXT_SENS_DATA_13 0x56
#define MPU6050_RA_EXT_SENS_DATA_14 0x57
#define MPU6050_RA_EXT_SENS_DATA_15 0x58
#define MPU6050_RA_EXT_SENS_DATA_16 0x59
#define MPU6050_RA_EXT_SENS_DATA_17 0x5A
#define MPU6050_RA_EXT_SENS_DATA_18 0x5B
#define MPU6050_RA_EXT_SENS_DATA_19 0x5C
#define MPU6050_RA_EXT_SENS_DATA_20 0x5D
#define MPU6050_RA_EXT_SENS_DATA_21 0x5E
#define MPU6050_RA_EXT_SENS_DATA_22 0x5F
#define MPU6050_RA_EXT_SENS_DATA_23 0x60
#define MPU6050_RA_MOT_DETECT_STATUS    0x61
#define MPU6050_RA_I2C_SLV0_DO      0x63
#define MPU6050_RA_I2C_SLV1_DO      0x64
#define MPU6050_RA_I2C_SLV2_DO      0x65
#define MPU6050_RA_I2C_SLV3_DO      0x66
#define MPU6050_RA_I2C_MST_DELAY_CTRL   0x67
#define MPU6050_RA_SIGNAL_PATH_RESET    0x68
#define MPU6050_RA_MOT_DETECT_CTRL      0x69
#define MPU6050_RA_USER_CTRL        0x6A
#define MPU6050_RA_PWR_MGMT_1       0x6B
#define MPU6050_RA_PWR_MGMT_2       0x6C
#define MPU6050_RA_BANK_SEL         0x6D
#define MPU6050_RA_MEM_START_ADDR   0x6E
#define MPU6050_RA_MEM_R_W          0x6F
#define MPU6050_RA_DMP_CFG_1        0x70
#define MPU6050_RA_DMP_CFG_2        0x71
#define MPU6050_RA_FIFO_COUNTH      0x72
#define MPU6050_RA_FIFO_COUNTL      0x73
#define MPU6050_RA_FIFO_R_W         0x74
#define MPU6050_RA_WHO_AM_I         0x75

#define MPU6050_TC_PWR_MODE_BIT     7
#define MPU6050_TC_OFFSET_BIT       6
#define MPU6050_TC_OFFSET_LENGTH    6
#define MPU6050_TC_OTP_BNK_VLD_BIT  0

#define MPU6050_VDDIO_LEVEL_VLOGIC  0
#define MPU6050_VDDIO_LEVEL_VDD     1

#define MPU6050_CFG_EXT_SYNC_SET_BIT    5
#define MPU6050_CFG_EXT_SYNC_SET_LENGTH 3
#define MPU6050_CFG_DLPF_CFG_BIT    2
#define MPU6050_CFG_DLPF_CFG_LENGTH 3

#define MPU6050_EXT_SYNC_DISABLED       0x0
#define MPU6050_EXT_SYNC_TEMP_OUT_L     0x1
#define MPU6050_EXT_SYNC_GYRO_XOUT_L    0x2
#define MPU6050_EXT_SYNC_GYRO_YOUT_L    0x3
#define MPU6050_EXT_SYNC_GYRO_ZOUT_L    0x4
#define MPU6050_EXT_SYNC_ACCEL_XOUT_L   0x5
#define MPU6050_EXT_SYNC_ACCEL_YOUT_L   0x6
#define MPU6050_EXT_SYNC_ACCEL_ZOUT_L   0x7

#define MPU6050_DLPF_BW_256         0x00
#define MPU6050_DLPF_BW_188         0x01
#define MPU6050_DLPF_BW_98          0x02
#define MPU6050_DLPF_BW_42          0x03
#define MPU6050_DLPF_BW_20          0x04
#define MPU6050_DLPF_BW_10          0x05
#define MPU6050_DLPF_BW_5           0x06

#define MPU6050_GCONFIG_XG_ST_BIT       7
#define MPU6050_GCONFIG_YG_ST_BIT       6
#define MPU6050_GCONFIG_ZG_ST_BIT       5
#define MPU6050_GCONFIG_FS_SEL_BIT      4
#define MPU6050_GCONFIG_FS_SEL_LENGTH   2


#define MPU6050_GYRO_FS_250         0x00
#define MPU6050_GYRO_FS_500         0x01
#define MPU6050_GYRO_FS_1000        0x02
#define MPU6050_GYRO_FS_2000        0x03

#define MPU6050_ACONFIG_XA_ST_BIT           7
#define MPU6050_ACONFIG_YA_ST_BIT           6
#define MPU6050_ACONFIG_ZA_ST_BIT           5
#define MPU6050_ACONFIG_AFS_SEL_BIT         4
#define MPU6050_ACONFIG_AFS_SEL_LENGTH      2
#define MPU6050_ACONFIG_ACCEL_HPF_BIT       2
#define MPU6050_ACONFIG_ACCEL_HPF_LENGTH    3

#define MPU6050_ACCEL_FS_2          0x00
#define MPU6050_ACCEL_FS_4          0x01
#define MPU6050_ACCEL_FS_8          0x02
#define MPU6050_ACCEL_FS_16         0x03

#define MPU6050_DHPF_RESET          0x00
#define MPU6050_DHPF_5              0x01
#define MPU6050_DHPF_2P5            0x02
#define MPU6050_DHPF_1P25           0x03
#define MPU6050_DHPF_0P63           0x04
#define MPU6050_DHPF_HOLD           0x07

#define MPU6050_TEMP_FIFO_EN_BIT    7
#define MPU6050_XG_FIFO_EN_BIT      6
#define MPU6050_YG_FIFO_EN_BIT      5
#define MPU6050_ZG_FIFO_EN_BIT      4
#define MPU6050_ACCEL_FIFO_EN_BIT   3
#define MPU6050_SLV2_FIFO_EN_BIT    2
#define MPU6050_SLV1_FIFO_EN_BIT    1
#define MPU6050_SLV0_FIFO_EN_BIT    0

#define MPU6050_MULT_MST_EN_BIT     7
#define MPU6050_WAIT_FOR_ES_BIT     6
#define MPU6050_SLV_3_FIFO_EN_BIT   5
#define MPU6050_I2C_MST_P_NSR_BIT   4
#define MPU6050_I2C_MST_CLK_BIT     3
#define MPU6050_I2C_MST_CLK_LENGTH  4

#define MPU6050_CLOCK_DIV_348       0x0
#define MPU6050_CLOCK_DIV_333       0x1
#define MPU6050_CLOCK_DIV_320       0x2
#define MPU6050_CLOCK_DIV_308       0x3
#define MPU6050_CLOCK_DIV_296       0x4
#define MPU6050_CLOCK_DIV_286       0x5
#define MPU6050_CLOCK_DIV_276       0x6
#define MPU6050_CLOCK_DIV_267       0x7
#define MPU6050_CLOCK_DIV_258       0x8
#define MPU6050_CLOCK_DIV_500       0x9
#define MPU6050_CLOCK_DIV_471       0xA
#define MPU6050_CLOCK_DIV_444       0xB
#define MPU6050_CLOCK_DIV_421       0xC
#define MPU6050_CLOCK_DIV_400       0xD
#define MPU6050_CLOCK_DIV_381       0xE
#define MPU6050_CLOCK_DIV_364       0xF

#define MPU6050_I2C_SLV_RW_BIT      7
#define MPU6050_I2C_SLV_ADDR_BIT    6
#define MPU6050_I2C_SLV_ADDR_LENGTH 7
#define MPU6050_I2C_SLV_EN_BIT      7
#define MPU6050_I2C_SLV_BYTE_SW_BIT 6
#define MPU6050_I2C_SLV_REG_DIS_BIT 5
#define MPU6050_I2C_SLV_GRP_BIT     4
#define MPU6050_I2C_SLV_LEN_BIT     3
#define MPU6050_I2C_SLV_LEN_LENGTH  4

#define MPU6050_I2C_SLV4_RW_BIT         7
#define MPU6050_I2C_SLV4_ADDR_BIT       6
#define MPU6050_I2C_SLV4_ADDR_LENGTH    7
#define MPU6050_I2C_SLV4_EN_BIT         7
#define MPU6050_I2C_SLV4_INT_EN_BIT     6
#define MPU6050_I2C_SLV4_REG_DIS_BIT    5
#define MPU6050_I2C_SLV4_MST_DLY_BIT    4
#define MPU6050_I2C_SLV4_MST_DLY_LENGTH 5

#define MPU6050_MST_PASS_THROUGH_BIT    7
#define MPU6050_MST_I2C_SLV4_DONE_BIT   6
#define MPU6050_MST_I2C_LOST_ARB_BIT    5
#define MPU6050_MST_I2C_SLV4_NACK_BIT   4
#define MPU6050_MST_I2C_SLV3_NACK_BIT   3
#define MPU6050_MST_I2C_SLV2_NACK_BIT   2
#define MPU6050_MST_I2C_SLV1_NACK_BIT   1
#define MPU6050_MST_I2C_SLV0_NACK_BIT   0

#define MPU6050_INTCFG_INT_LEVEL_BIT        7
#define MPU6050_INTCFG_INT_OPEN_BIT         6
#define MPU6050_INTCFG_LATCH_INT_EN_BIT     5
#define MPU6050_INTCFG_INT_RD_CLEAR_BIT     4
#define MPU6050_INTCFG_FSYNC_INT_LEVEL_BIT  3
#define MPU6050_INTCFG_FSYNC_INT_EN_BIT     2
#define MPU6050_INTCFG_I2C_BYPASS_EN_BIT    1
#define MPU6050_INTCFG_CLKOUT_EN_BIT        0

#define MPU6050_INTMODE_ACTIVEHIGH  0x00
#define MPU6050_INTMODE_ACTIVELOW   0x01

#define MPU6050_INTDRV_PUSHPULL     0x00
#define MPU6050_INTDRV_OPENDRAIN    0x01

#define MPU6050_INTLATCH_50USPULSE  0x00
#define MPU6050_INTLATCH_WAITCLEAR  0x01

#define MPU6050_INTCLEAR_STATUSREAD 0x00
#define MPU6050_INTCLEAR_ANYREAD    0x01

#define MPU6050_INTERRUPT_FF_BIT            7
#define MPU6050_INTERRUPT_MOT_BIT           6
#define MPU6050_INTERRUPT_ZMOT_BIT          5
#define MPU6050_INTERRUPT_FIFO_OFLOW_BIT    4
#define MPU6050_INTERRUPT_I2C_MST_INT_BIT   3
#define MPU6050_INTERRUPT_PLL_RDY_INT_BIT   2
#define MPU6050_INTERRUPT_DMP_INT_BIT       1
#define MPU6050_INTERRUPT_DATA_RDY_BIT      0

// TODO: figure out what these actually do
// UMPL source code is not very obivous
#define MPU6050_DMPINT_5_BIT            5
#define MPU6050_DMPINT_4_BIT            4
#define MPU6050_DMPINT_3_BIT            3
#define MPU6050_DMPINT_2_BIT            2
#define MPU6050_DMPINT_1_BIT            1
#define MPU6050_DMPINT_0_BIT            0

#define MPU6050_MOTION_MOT_XNEG_BIT     7
#define MPU6050_MOTION_MOT_XPOS_BIT     6
#define MPU6050_MOTION_MOT_YNEG_BIT     5
#define MPU6050_MOTION_MOT_YPOS_BIT     4
#define MPU6050_MOTION_MOT_ZNEG_BIT     3
#define MPU6050_MOTION_MOT_ZPOS_BIT     2
#define MPU6050_MOTION_MOT_ZRMOT_BIT    0

#define MPU6050_DELAYCTRL_DELAY_ES_SHADOW_BIT   7
#define MPU6050_DELAYCTRL_I2C_SLV4_DLY_EN_BIT   4
#define MPU6050_DELAYCTRL_I2C_SLV3_DLY_EN_BIT   3
#define MPU6050_DELAYCTRL_I2C_SLV2_DLY_EN_BIT   2
#define MPU6050_DELAYCTRL_I2C_SLV1_DLY_EN_BIT   1
#define MPU6050_DELAYCTRL_I2C_SLV0_DLY_EN_BIT   0

#define MPU6050_PATHRESET_GYRO_RESET_BIT    2
#define MPU6050_PATHRESET_ACCEL_RESET_BIT   1
#define MPU6050_PATHRESET_TEMP_RESET_BIT    0

#define MPU6050_DETECT_ACCEL_ON_DELAY_BIT       5
#define MPU6050_DETECT_ACCEL_ON_DELAY_LENGTH    2
#define MPU6050_DETECT_FF_COUNT_BIT             3
#define MPU6050_DETECT_FF_COUNT_LENGTH          2
#define MPU6050_DETECT_MOT_COUNT_BIT            1
#define MPU6050_DETECT_MOT_COUNT_LENGTH         2

#define MPU6050_DETECT_DECREMENT_RESET  0x0
#define MPU6050_DETECT_DECREMENT_1      0x1
#define MPU6050_DETECT_DECREMENT_2      0x2
#define MPU6050_DETECT_DECREMENT_4      0x3

#define MPU6050_USERCTRL_DMP_EN_BIT             7
#define MPU6050_USERCTRL_FIFO_EN_BIT            6
#define MPU6050_USERCTRL_I2C_MST_EN_BIT         5
#define MPU6050_USERCTRL_I2C_IF_DIS_BIT         4
#define MPU6050_USERCTRL_DMP_RESET_BIT          3
#define MPU6050_USERCTRL_FIFO_RESET_BIT         2
#define MPU6050_USERCTRL_I2C_MST_RESET_BIT      1
#define MPU6050_USERCTRL_SIG_COND_RESET_BIT     0

#define MPU6050_PWR1_DEVICE_RESET_BIT   7
#define MPU6050_PWR1_SLEEP_BIT          6
#define MPU6050_PWR1_CYCLE_BIT          5
#define MPU6050_PWR1_TEMP_DIS_BIT       3
#define MPU6050_PWR1_CLKSEL_BIT         2
#define MPU6050_PWR1_CLKSEL_LENGTH      3

#define MPU6050_CLOCK_INTERNAL          0x00
#define MPU6050_CLOCK_PLL_XGYRO         0x01
#define MPU6050_CLOCK_PLL_YGYRO         0x02
#define MPU6050_CLOCK_PLL_ZGYRO         0x03
#define MPU6050_CLOCK_PLL_EXT32K        0x04
#define MPU6050_CLOCK_PLL_EXT19M        0x05
#define MPU6050_CLOCK_KEEP_RESET        0x07

#define MPU6050_PWR2_LP_WAKE_CTRL_BIT       7
#define MPU6050_PWR2_LP_WAKE_CTRL_LENGTH    2
#define MPU6050_PWR2_STBY_XA_BIT            5
#define MPU6050_PWR2_STBY_YA_BIT            4
#define MPU6050_PWR2_STBY_ZA_BIT            3
#define MPU6050_PWR2_STBY_XG_BIT            2
#define MPU6050_PWR2_STBY_YG_BIT            1
#define MPU6050_PWR2_STBY_ZG_BIT            0

#define MPU6050_WAKE_FREQ_1P25      0x0
#define MPU6050_WAKE_FREQ_2P5       0x1
#define MPU6050_WAKE_FREQ_5         0x2
#define MPU6050_WAKE_FREQ_10        0x3

#define MPU6050_BANKSEL_PRFTCH_EN_BIT       6
#define MPU6050_BANKSEL_CFG_USER_BANK_BIT   5
#define MPU6050_BANKSEL_MEM_SEL_BIT         4
#define MPU6050_BANKSEL_MEM_SEL_LENGTH      5

#define MPU6050_WHO_AM_I_BIT        6
#define MPU6050_WHO_AM_I_LENGTH     6

#define MPU6050_DMP_MEMORY_BANKS        8
#define MPU6050_DMP_MEMORY_BANK_SIZE    256
#define MPU6050_DMP_MEMORY_CHUNK_SIZE   16

#define MPU6050_DEG_PER_LSB_250  (float)((2 * 250.0) / 65536.0)
#define MPU6050_DEG_PER_LSB_500  (float)((2 * 500.0) / 65536.0)
#define MPU6050_DEG_PER_LSB_1000 (float)((2 * 1000.0) / 65536.0)
#define MPU6050_DEG_PER_LSB_2000 (float)((2 * 2000.0) / 65536.0)

#define MPU6050_G_PER_LSB_2      (float)((2 * 2) / 65536.0)
#define MPU6050_G_PER_LSB_4      (float)((2 * 4) / 65536.0)
#define MPU6050_G_PER_LSB_8      (float)((2 * 8) / 65536.0)
#define MPU6050_G_PER_LSB_16     (float)((2 * 16) / 65536.0)

#define MPU6050_ST_GYRO_LOW      10.0   // deg/s
#define MPU6050_ST_GYRO_HIGH     105.0  // deg/s
#define MPU6050_ST_ACCEL_LOW     0.300  // G
#define MPU6050_ST_ACCEL_HIGH    0.950  // G

// note: DMP code memory blocks defined at end of header file

void mpu6050Init(I2C_Dev *i2cPort);
void mpu6050Deinit(void);
bool mpu6050Test(void);

bool mpu6050TestConnection();
bool mpu6050EvaluateSelfTest(float low, float high, float value, char *string);
bool mpu6050SelfTest();


// AUX_VDDIO register
uint8_t mpu6050GetAuxVDDIOLevel();
void mpu6050SetAuxVDDIOLevel(uint8_t level);

// SMPLRT_DIV register
uint8_t mpu6050GetRate();
void mpu6050SetRate(uint8_t rate);

// CONFIG register
uint8_t mpu6050GetExternalFrameSync();
void mpu6050SetExternalFrameSync(uint8_t sync);
uint8_t mpu6050GetDLPFMode();
void mpu6050SetDLPFMode(uint8_t bandwidth);

// GYRO_CONFIG register
void mpu6050SetGyroXSelfTest(bool enabled);
void mpu6050SetGyroYSelfTest(bool enabled);
void mpu6050SetGyroZSelfTest(bool enabled);
uint8_t mpu6050GetFullScaleGyroRangeId();
float mpu6050GetFullScaleGyroDPL();
void mpu6050SetFullScaleGyroRange(uint8_t range);

// ACCEL_CONFIG register
bool mpu6050GetAccelXSelfTest();
void mpu6050SetAccelXSelfTest(bool enabled);
bool mpu6050GetAccelYSelfTest();
void mpu6050SetAccelYSelfTest(bool enabled);
bool mpu6050GetAccelZSelfTest();
void mpu6050SetAccelZSelfTest(bool enabled);
uint8_t mpu6050GetFullScaleAccelRangeId();
void mpu6050SetFullScaleAccelRange(uint8_t range);
float mpu6050GetFullScaleAccelGPL();
uint8_t mpu6050GetDHPFMode();
void mpu6050SetDHPFMode(uint8_t mode);

// FF_THR register
uint8_t mpu6050GetFreefallDetectionThreshold();
void mpu6050SetFreefallDetectionThreshold(uint8_t threshold);

// FF_DUR register
uint8_t mpu6050GetFreefallDetectionDuration();
void mpu6050SetFreefallDetectionDuration(uint8_t duration);

// MOT_THR register
uint8_t mpu6050GetMotionDetectionThreshold();
void mpu6050SetMotionDetectionThreshold(uint8_t threshold);

// MOT_DUR register
uint8_t mpu6050GetMotionDetectionDuration();
void mpu6050SetMotionDetectionDuration(uint8_t duration);

// ZRMOT_THR register
uint8_t mpu6050GetZeroMotionDetectionThreshold();
void mpu6050SetZeroMotionDetectionThreshold(uint8_t threshold);

// ZRMOT_DUR register
uint8_t mpu6050GetZeroMotionDetectionDuration();
void mpu6050SetZeroMotionDetectionDuration(uint8_t duration);

// FIFO_EN register
bool mpu6050GetTempFIFOEnabled();
void mpu6050SetTempFIFOEnabled(bool enabled);
bool mpu6050GetXGyroFIFOEnabled();
void mpu6050SetXGyroFIFOEnabled(bool enabled);
bool mpu6050GetYGyroFIFOEnabled();
void mpu6050SetYGyroFIFOEnabled(bool enabled);
bool mpu6050GetZGyroFIFOEnabled();
void mpu6050SetZGyroFIFOEnabled(bool enabled);
bool mpu6050GetAccelFIFOEnabled();
void mpu6050SetAccelFIFOEnabled(bool enabled);
bool mpu6050GetSlave2FIFOEnabled();
void mpu6050SetSlave2FIFOEnabled(bool enabled);
bool mpu6050GetSlave1FIFOEnabled();
void mpu6050SetSlave1FIFOEnabled(bool enabled);
bool mpu6050GetSlave0FIFOEnabled();
void mpu6050SetSlave0FIFOEnabled(bool enabled);

// I2C_MST_CTRL register
bool mpu6050GetMultiMasterEnabled();
void mpu6050SetMultiMasterEnabled(bool enabled);
bool mpu6050GetWaitForExternalSensorEnabled();
void mpu6050SetWaitForExternalSensorEnabled(bool enabled);
bool mpu6050GetSlave3FIFOEnabled();
void mpu6050SetSlave3FIFOEnabled(bool enabled);
bool mpu6050GetSlaveReadWriteTransitionEnabled();
void mpu6050SetSlaveReadWriteTransitionEnabled(bool enabled);
uint8_t mpu6050GetMasterClockSpeed();
void mpu6050SetMasterClockSpeed(uint8_t speed);

// I2C_SLV* registers (Slave 0-3)
uint8_t mpu6050GetSlaveAddress(uint8_t num);
void mpu6050SetSlaveAddress(uint8_t num, uint8_t address);
uint8_t mpu6050GetSlaveRegister(uint8_t num);
void mpu6050SetSlaveRegister(uint8_t num, uint8_t reg);
bool mpu6050GetSlaveEnabled(uint8_t num);
void mpu6050SetSlaveEnabled(uint8_t num, bool enabled);
bool mpu6050GetSlaveWordByteSwap(uint8_t num);
void mpu6050SetSlaveWordByteSwap(uint8_t num, bool enabled);
bool mpu6050GetSlaveWriteMode(uint8_t num);
void mpu6050SetSlaveWriteMode(uint8_t num, bool mode);
bool mpu6050GetSlaveWordGroupOffmpu6050Set(uint8_t num);
void setSlaveWordGroupOffset(uint8_t num, bool enabled);
uint8_t mpu6050GetSlaveDataLength(uint8_t num);
void mpu6050SetSlaveDataLength(uint8_t num, uint8_t length);

// I2C_SLV* registers (Slave 4)
uint8_t mpu6050GetSlave4Address();
void mpu6050SetSlave4Address(uint8_t address);
uint8_t mpu6050GetSlave4Register();
void mpu6050SetSlave4Register(uint8_t reg);
void mpu6050SetSlave4OutputByte(uint8_t data);
bool mpu6050GetSlave4Enabled();
void mpu6050SetSlave4Enabled(bool enabled);
bool mpu6050GetSlave4InterruptEnabled();
void mpu6050SetSlave4InterruptEnabled(bool enabled);
bool mpu6050GetSlave4WriteMode();
void mpu6050SetSlave4WriteMode(bool mode);
uint8_t mpu6050GetSlave4MasterDelay();
void mpu6050SetSlave4MasterDelay(uint8_t delay);
uint8_t mpu6050GetSlate4InputByte();

// I2C_MST_STATUS register
bool mpu6050GetPassthroughStatus();
bool mpu6050GetSlave4IsDone();
bool mpu6050GetLostArbitration();
bool mpu6050GetSlave4Nack();
bool mpu6050GetSlave3Nack();
bool mpu6050GetSlave2Nack();
bool mpu6050GetSlave1Nack();
bool mpu6050GetSlave0Nack();

// INT_PIN_CFG register
bool mpu6050GetInterruptMode();
void mpu6050SetInterruptMode(bool mode);
bool mpu6050GetInterruptDrive();
void mpu6050SetInterruptDrive(bool drive);
bool mpu6050GetInterruptLatch();
void mpu6050SetInterruptLatch(bool latch);
bool mpu6050GetInterruptLatchClear();
void mpu6050SetInterruptLatchClear(bool clear);
bool mpu6050GetFSyncInterruptLevel();
void mpu6050SetFSyncInterruptLevel(bool level);
bool mpu6050GetFSyncInterruptEnabled();
void mpu6050SetFSyncInterruptEnabled(bool enabled);
bool mpu6050GetI2CBypassEnabled();
void mpu6050SetI2CBypassEnabled(bool enabled);
bool mpu6050GetClockOutputEnabled();
void mpu6050SetClockOutputEnabled(bool enabled);

// INT_ENABLE register
uint8_t mpu6050GetIntEnabled();
void mpu6050SetIntEnabled(uint8_t enabled);
bool mpu6050GetIntFreefallEnabled();
void mpu6050SetIntFreefallEnabled(bool enabled);
bool mpu6050GetIntMotionEnabled();
void mpu6050SetIntMotionEnabled(bool enabled);
bool mpu6050GetIntZeroMotionEnabled();
void mpu6050SetIntZeroMotionEnabled(bool enabled);
bool mpu6050GetIntFIFOBufferOverflowEnabled();
void mpu6050SetIntFIFOBufferOverflowEnabled(bool enabled);
bool mpu6050GetIntI2CMasterEnabled();
void mpu6050SetIntI2CMasterEnabled(bool enabled);
bool mpu6050GetIntDataReadyEnabled();
void mpu6050SetIntDataReadyEnabled(bool enabled);

// INT_STATUS register
uint8_t mpu6050GetIntStatus();
bool mpu6050GetIntFreefallStatus();
bool mpu6050GetIntMotionStatus();
bool mpu6050GetIntZeroMotionStatus();
bool mpu6050GetIntFIFOBufferOverflowStatus();
bool mpu6050GetIntI2CMasterStatus();
bool mpu6050GetIntDataReadyStatus();

// ACCEL_*OUT_* registers
void mpu6050GetMotion9(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz, int16_t *mx, int16_t *my, int16_t *mz);
void mpu6050GetMotion6(int16_t *ax, int16_t *ay, int16_t *az, int16_t *gx, int16_t *gy, int16_t *gz);
void mpu6050GetAcceleration(int16_t *x, int16_t *y, int16_t *z);
int16_t mpu6050GetAccelerationX();
int16_t mpu6050GetAccelerationY();
int16_t mpu6050GetAccelerationZ();

// TEMP_OUT_* registers
int16_t mpu6050GetTemperature();

// GYRO_*OUT_* registers
void mpu6050GetRotation(int16_t *x, int16_t *y, int16_t *z);
int16_t mpu6050GetRotationX();
int16_t mpu6050GetRotationY();
int16_t mpu6050GetRotationZ();

// EXT_SENS_DATA_* registers
uint8_t mpu6050GetExternalSensorByte(int position);
uint16_t mpu6050GetExternalSensorWord(int position);
uint32_t mpu6050GetExternalSensorDWord(int position);

// MOT_DETECT_STATUS register
bool mpu6050GetXNegMotionDetected();
bool mpu6050GetXPosMotionDetected();
bool mpu6050GetYNegMotionDetected();
bool mpu6050GetYPosMotionDetected();
bool mpu6050GetZNegMotionDetected();
bool mpu6050GetZPosMotionDetected();
bool mpu6050GetZeroMotionDetected();

// I2C_SLV*_DO register
void mpu6050SetSlaveOutputByte(uint8_t num, uint8_t data);

// I2C_MST_DELAY_CTRL register
bool mpu6050GetExternalShadowDelayEnabled();
void mpu6050SetExternalShadowDelayEnabled(bool enabled);
bool mpu6050GetSlaveDelayEnabled(uint8_t num);
void mpu6050SetSlaveDelayEnabled(uint8_t num, bool enabled);

// SIGNAL_PATH_RESET register
void rempu6050SetGyroscopePath();
void rempu6050SetAccelerometerPath();
void rempu6050SetTemperaturePath();

// MOT_DETECT_CTRL register
uint8_t mpu6050GetAccelerometerPowerOnDelay();
void mpu6050SetAccelerometerPowerOnDelay(uint8_t delay);
uint8_t mpu6050GetFreefallDetectionCounterDecrement();
void mpu6050SetFreefallDetectionCounterDecrement(uint8_t decrement);
uint8_t mpu6050GetMotionDetectionCounterDecrement();
void mpu6050SetMotionDetectionCounterDecrement(uint8_t decrement);

// USER_CTRL register
bool mpu6050GetFIFOEnabled();
void mpu6050SetFIFOEnabled(bool enabled);
bool mpu6050GetI2CMasterModeEnabled();
void mpu6050SetI2CMasterModeEnabled(bool enabled);
void mpu6050SwitchSPIEnabled(bool enabled);
void mpu6050ResetFIFO();
void mpu6050ResetI2CMaster();
void mpu6050ResetSensors();

// PWR_MGMT_1 register
void mpu6050Reset();
bool mpu6050GetSleepEnabled();
void mpu6050SetSleepEnabled(bool enabled);
bool mpu6050GetWakeCycleEnabled();
void mpu6050SetWakeCycleEnabled(bool enabled);
bool mpu6050GetTempSensorEnabled();
void mpu6050SetTempSensorEnabled(bool enabled);
uint8_t mpu6050GetClockSource();
void mpu6050SetClockSource(uint8_t source);

// PWR_MGMT_2 register
uint8_t mpu6050GetWakeFrequency();
void mpu6050SetWakeFrequency(uint8_t frequency);
bool mpu6050GetStandbyXAccelEnabled();
void mpu6050SetStandbyXAccelEnabled(bool enabled);
bool mpu6050GetStandbyYAccelEnabled();
void mpu6050SetStandbyYAccelEnabled(bool enabled);
bool mpu6050GetStandbyZAccelEnabled();
void mpu6050SetStandbyZAccelEnabled(bool enabled);
bool mpu6050GetStandbyXGyroEnabled();
void mpu6050SetStandbyXGyroEnabled(bool enabled);
bool mpu6050GetStandbyYGyroEnabled();
void mpu6050SetStandbyYGyroEnabled(bool enabled);
bool mpu6050GetStandbyZGyroEnabled();
void mpu6050SetStandbyZGyroEnabled(bool enabled);

// FIFO_COUNT_* registers
uint16_t mpu6050GetFIFOCount();

// FIFO_R_W register
uint8_t mpu6050GetFIFOByte();
void mpu6050SetFIFOByte(uint8_t data);
void mpu6050GetFIFOBytes(uint8_t *data, uint8_t length);

// WHO_AM_I register
uint8_t mpu6050GetDeviceID();
void mpu6050SetDeviceID(uint8_t id);

// ======== UNDOCUMENTED/DMP REGISTERS/METHODS ========

// XG_OFFS_TC register
uint8_t mpu6050GetOTPBankValid();
void mpu6050SetOTPBankValid(bool enabled);
int8_t mpu6050GetXGyroOffset();
void mpu6050SetXGyroOffset(int8_t offset);

// YG_OFFS_TC register
int8_t mpu6050GetYGyroOffset();
void mpu6050SetYGyroOffset(int8_t offset);

// ZG_OFFS_TC register
int8_t mpu6050GetZGyroOffset();
void  mpu6050SetGyroOffset(int8_t offset);

// X_FINE_GAIN register
int8_t mpu6050GetXFineGain();
void mpu6050SetXFineGain(int8_t gain);

// Y_FINE_GAIN register
int8_t mpu6050GetYFineGain();
void mpu6050SetYFineGain(int8_t gain);

// Z_FINE_GAIN register
int8_t mpu6050GetZFineGain();
void mpu6050SetZFineGain(int8_t gain);

// XA_OFFS_* registers
int16_t mpu6050GetXAccelOffset();
void mpu6050SetXAccelOffset(int16_t offset);

// YA_OFFS_* register
int16_t mpu6050GetYAccelOffset();
void mpu6050SetYAccelOffset(int16_t offset);

// ZA_OFFS_* register
int16_t mpu6050GetZAccelOffset();
void mpu6050SetZAccelOffset(int16_t offset);

// XG_OFFS_USR* registers
int16_t mpu6050GetXGyroOffsetUser();
void mpu6050SetXGyroOffsetUser(int16_t offset);

// YG_OFFS_USR* register
int16_t mpu6050GetYGyroOffsetUser();
void mpu6050SetYGyroOffsetUser(int16_t offset);

// ZG_OFFS_USR* register
int16_t mpu6050GetZGyroOffsetUser();
void mpu6050SetZGyroOffsetUser(int16_t offset);

// INT_ENABLE register (DMP functions)
bool mpu6050GetIntPLLReadyEnabled();
void mpu6050SetIntPLLReadyEnabled(bool enabled);
bool mpu6050GetIntDMPEnabled();
void mpu6050SetIntDMPEnabled(bool enabled);

// DMP_INT_STATUS
bool mpu6050GetDMPInt5Status();
bool mpu6050GetDMPInt4Status();
bool mpu6050GetDMPInt3Status();
bool mpu6050GetDMPInt2Status();
bool mpu6050GetDMPInt1Status();
bool mpu6050GetDMPInt0Status();

// INT_STATUS register (DMP functions)
bool mpu6050GetIntPLLReadyStatus();
bool mpu6050GetIntDMPStatus();

// USER_CTRL register (DMP functions)
bool mpu6050GetDMPEnabled();
void mpu6050SetDMPEnabled(bool enabled);
void mpu6050ResetDMP();

// BANK_SEL register
void mpu6050SetMemoryBank(uint8_t bank, bool prefetchEnabled, bool userBank);

// MEM_START_ADDR register
void mpu6050SetMemoryStartAddress(uint8_t address);

// MEM_R_W register
uint8_t mpu6050ReadMemoryByte();
void mpu6050WriteMemoryByte(uint8_t data);
void mpu6050ReadMemoryBlock(uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address);
bool mpu6050WriteMemoryBlock(const uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address, bool verify);
bool mpu6050WriteProgMemoryBlock(const uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address, bool verify);

bool mpu6050WriteDMPConfigurationSet(const uint8_t *data, uint16_t dataSize);
bool mpu6050WriteProgDMPConfigurationSet(const uint8_t *data, uint16_t dataSize);

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mpu6050VectorInt16_t;

typedef struct {
    float x;
    float y;
    float z;
} mpu6050VectorFloat_t;

typedef struct {
    float w;
    float x;
    float y;
    float z;
} mpu6050Quaternion_t;

// ==== DMP functions ==========================================================
// Ported from i2cdev: MPU6050_6Axis_MotionApps20.cpp/.h

uint8_t mpu6050DmpInitialize(void);

// ---- Custom API ---------------------------------------------
// These functions act more as "orchestrators", managing sequences.
void mpu6050DmpBootstrap(void);
void mpu6050Configure(void); // TODO; pass config
void mpu6050DmpConfigure(void);
void mpu6050DmpEnable(void);
bool mpu6050DmpLoadFirmware(void);
// -------------------------------------------------------------

bool mpu6050DmpPacketAvailable(void);
uint16_t mpu6050DmpGetFIFOPacketSize(void);
uint8_t mpu6050DmpGetCurrentFIFOPacket(uint8_t *data);
uint8_t mpu6050DmpGetAccelInt32(int32_t *data, const uint8_t *packet);
uint8_t mpu6050DmpGetAccelInt16(int16_t *data, const uint8_t *packet);
uint8_t mpu6050DmpGetAccel(mpu6050VectorInt16_t *v, const uint8_t *packet);
uint8_t mpu6050DmpGetQuaternionInt32(int32_t *data, const uint8_t *packet);
uint8_t mpu6050DmpGetQuaternionInt16(int16_t *data, const uint8_t *packet);
uint8_t mpu6050DmpGetQuaternion(mpu6050Quaternion_t *q, const uint8_t *packet);
uint8_t mpu6050DmpGetGyroInt32(int32_t *data, const uint8_t *packet);
uint8_t mpu6050DmpGetGyroInt16(int16_t *data, const uint8_t *packet);
uint8_t mpu6050DmpGetGyro(mpu6050VectorInt16_t *v, const uint8_t *packet);
uint8_t mpu6050DmpGetGravityInt16(int16_t *data, const uint8_t *packet);
uint8_t mpu6050DmpGetGravity(mpu6050VectorFloat_t *v, const mpu6050Quaternion_t *q);
uint8_t mpu6050DmpGetLinearAccel(mpu6050VectorInt16_t *v, const mpu6050VectorInt16_t *vRaw, const mpu6050VectorFloat_t *gravity);
uint8_t mpu6050DmpGetEuler(float *data, const mpu6050Quaternion_t *q);
uint8_t mpu6050DmpGetYawPitchRoll(float *data, const mpu6050Quaternion_t *q, const mpu6050VectorFloat_t *gravity);

// DMP_CFG_1 register
uint8_t mpu6050GetDMPConfig1();
void mpu6050SetDMPConfig1(uint8_t config);

// DMP_CFG_2 register
uint8_t mpu6050GetDMPConfig2();
void mpu6050SetDMPConfig2(uint8_t config);

// HACK: REMOVE THIS LATER
#define MPU6050_INCLUDE_DMP_MOTIONAPPS20 // NOTE: required for enabling DMP!!!

#ifdef MPU6050_INCLUDE_DMP_MOTIONAPPS20
/* This is only included if you want it, since it eats about 2K of program
 * memory, which is a waste if you aren't using the DMP (or if you aren't
 * using this particular flavor of DMP).
 *
 * Source is from the InvenSense MotionApps v2 demo code. Original source is
 * unavailable, unless you happen to be amazing as decompiling binary by
 * hand (in which case, please contact me, and I'm totally serious).
 *
 * Also, I'd like to offer many, many thanks to Noah Zerkin for all of the
 * DMP reverse-engineering he did to help make this bit of wizardry
 * possible.
 */

#define MPU6050_DMP_CODE_SIZE 1929
#define MPU6050_DMP_CONFIG_SIZE 192

// this block of memory gets written to the MPU on start-up, and it seems
// to be volatile memory, so it has to be done each time (it only takes ~1
// second though)
extern const uint8_t dmpMemory[MPU6050_DMP_CODE_SIZE];
extern const uint8_t dmpConfig[MPU6050_DMP_CONFIG_SIZE];

#endif

#ifdef __cplusplus
}
#endif

#endif /* _MPU6050_H_ */
