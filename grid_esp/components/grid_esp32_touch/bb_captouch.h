// BitBank Capacitive Touch Sensor Library
// Stripped to MXT144 only, non-Arduino ESP-IDF port
//
// Copyright 2023 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include <stdint.h>
#include "driver/i2c.h"

#define CT_SUCCESS  0
#define CT_ERROR   -1

#define MXT144_ADDR 0x4A

typedef struct mxt_data_tag {
    uint16_t t2_encryption_status_address;
    uint16_t t5_message_processor_address;
    uint16_t t5_max_message_size;
    uint16_t t6_command_processor_address;
    uint16_t t7_powerconfig_address;
    uint16_t t8_acquisitionconfig_address;
    uint16_t t37_diagnostic_address;
    uint16_t t44_message_count_address;
    uint16_t t46_cte_config_address;
    uint16_t t56_shieldless_address;
    uint16_t t15_key_array_address;
    uint16_t t15_first_report_id;
    uint16_t t100_multiple_touch_touchscreen_address;
    uint16_t t100_first_report_id;
    uint8_t  matrix_x_size;
    uint8_t  matrix_y_size;
} MXTDATA;

#define MXT_MESSAGE_SIZE 6

#ifndef __TOUCHINFO_STRUCT__
#define __TOUCHINFO_STRUCT__
typedef struct _fttouchinfo {
    int count;
    uint32_t key_state;
    uint16_t x[5], y[5];
    uint8_t pressure[5], area[5];
} TOUCHINFO;
#endif

class BBCapTouch {
public:
    BBCapTouch() {}
    int init(int iSDA, int iSCL, uint32_t u32Speed = 400000);
    int getSamples(TOUCHINFO *pTI);
    void dumpDiagnostic(void);

private:
    int _iAddr = MXT144_ADDR;
    MXTDATA _mxtdata = {};
    int initMXT(void);
    bool I2CTest(uint8_t u8Addr);
    int I2CRead(uint8_t u8Addr, uint8_t *pData, int iLen);
    int I2CReadRegister16(uint8_t u8Addr, uint16_t u16Register, uint8_t *pData, int iLen);
    int I2CWrite(uint8_t u8Addr, uint8_t *pData, int iLen);
};
