// BitBank Capacitive Touch Sensor Library
// Stripped to MXT144 only, non-Arduino ESP-IDF port
//
// Copyright 2023 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0

#include "bb_captouch.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//
// Initialize the MXT144
//
int BBCapTouch::initMXT(void) {
    uint8_t ucTemp[32];
    int i, iObjCount, iReportID;
    uint16_t u16, u16Offset;

    I2CReadRegister16(_iAddr, 0, ucTemp, 7);
    iObjCount = ucTemp[6];
    _mxtdata.matrix_x_size = ucTemp[4];
    _mxtdata.matrix_y_size = ucTemp[5];
    ets_printf("mxt initMXT: family=0x%02X variant=0x%02X fw=%d.%d obj_count=%d matrix=%dx%d\r\n",
               ucTemp[0], ucTemp[1], ucTemp[2], ucTemp[3], iObjCount,
               _mxtdata.matrix_x_size, _mxtdata.matrix_y_size);
    if (iObjCount < 1 || iObjCount > 64) {
        return CT_ERROR;
    }
    u16Offset = 7;
    iReportID = 1;
    for (i = 0; i < iObjCount; i++) {
        I2CReadRegister16(_iAddr, u16Offset, ucTemp, 6);
        u16 = ucTemp[1] | (ucTemp[2] << 8);
        ets_printf("  obj[%d] type=%d addr=0x%04X size=%d instances=%d rids=%d\r\n",
                   i, ucTemp[0], u16, ucTemp[3], ucTemp[4], ucTemp[5]);
        switch (ucTemp[0]) {
            case 2:  _mxtdata.t2_encryption_status_address = u16; break;
            case 5:
                _mxtdata.t5_message_processor_address = u16;
                _mxtdata.t5_max_message_size = ucTemp[3] + 1;  // actual size = size_field + 1
                break;
            case 6:  _mxtdata.t6_command_processor_address = u16; break;
            case 7:  _mxtdata.t7_powerconfig_address = u16; break;
            case 8:  _mxtdata.t8_acquisitionconfig_address = u16; break;
            case 15:
                _mxtdata.t15_key_array_address = u16;
                _mxtdata.t15_first_report_id = iReportID + 1; // +1: sensor allocates one status slot before key data
                break;
            case 37: _mxtdata.t37_diagnostic_address = u16; break;
            case 44: _mxtdata.t44_message_count_address = u16; break;
            case 46: _mxtdata.t46_cte_config_address = u16; break;
            case 56: _mxtdata.t56_shieldless_address = u16; break;
            case 100:
                _mxtdata.t100_multiple_touch_touchscreen_address = u16;
                _mxtdata.t100_first_report_id = iReportID;
                break;
            default: break;
        }
        u16Offset += 6;
        // Type 166 appears in the object table with rids=12/instances=12 but does NOT
        // allocate user-visible report IDs on this chip variant — skip its contribution.
        if (ucTemp[0] != 166) {
            iReportID += ucTemp[5] * (ucTemp[4] + 1);
        }
    }
    return CT_SUCCESS;
}

//
// Initialize the library — reset and I2C are already configured by grid_esp32_touch_init().
// This re-applies I2C config, detects the MXT144 and reads its object table.
//
int BBCapTouch::init(int iSDA, int iSCL, uint32_t u32Speed) {
    i2c_config_t conf = {};
    conf.mode             = I2C_MODE_MASTER;
    conf.sda_io_num       = iSDA;
    conf.scl_io_num       = iSCL;
    conf.sda_pullup_en    = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en    = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = u32Speed;
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    if (!I2CTest(MXT144_ADDR)) {
        return CT_ERROR;
    }
    int rc = initMXT();
    if (rc == CT_SUCCESS) {
        // T7: acquisition timing (free-run at 16ms intervals).
        {
            uint8_t t7wr[2 + 4] = {};
            t7wr[0] = (uint8_t)(_mxtdata.t7_powerconfig_address);
            t7wr[1] = (uint8_t)(_mxtdata.t7_powerconfig_address >> 8);
            t7wr[2] = 16;  // IDLEACQINT: 16ms
            t7wr[3] = 16;  // ACTVACQINT: 16ms
            I2CWrite(_iAddr, t7wr, 2 + 4);
        }

        // T8: charge time and measurement parameters (reference: Table 11-1).
        {
            uint8_t t8wr[2 + 14] = {};
            t8wr[0] = (uint8_t)(_mxtdata.t8_acquisitionconfig_address);
            t8wr[1] = (uint8_t)(_mxtdata.t8_acquisitionconfig_address >> 8);
            t8wr[2 +  0] = 26;  // CHRGTIME
            t8wr[2 + 10] = 11;  // MEASALLOW
            t8wr[2 + 11] =  8;  // MEASIDLEDEF
            t8wr[2 + 12] =  2;  // MEASACTVDEF
            I2CWrite(_iAddr, t8wr, 2 + 14);
        }

        // T46: CTE (charge transfer engine), reference: IDLESYNCSPERX=16, ACTSYNCSPERX=16.
        {
            uint8_t t46wr[2 + 11] = {};
            t46wr[0] = (uint8_t)(_mxtdata.t46_cte_config_address);
            t46wr[1] = (uint8_t)(_mxtdata.t46_cte_config_address >> 8);
            t46wr[2 + 0] = 0x01;  // CTRL: ENABLE
            t46wr[2 + 2] = 16;    // IDLESYNCSPERX
            t46wr[2 + 3] = 16;    // ACTIVESYNCSPERX
            t46wr[2 + 4] =  4;    // ADCSPERSYNC
            t46wr[2 + 5] =  1;    // PULSESPERADC
            I2CWrite(_iAddr, t46wr, 2 + 11);
        }

        // T7 must be written BEFORE T6 CALIBRATE so the scanner restarts with the correct interval.
        // (Blank NVM leaves IDLEACQINT=0 which stops the scanner after one measurement.)
        {
            uint8_t rb[4] = {};
            I2CReadRegister16(_iAddr, _mxtdata.t7_powerconfig_address, rb, 4);
            ets_printf("mxt T7 readback: IDLEACQINT=%d ACTVACQINT=%d ACTV2IDLETO=%d\r\n", rb[0], rb[1], rb[2]);
            I2CReadRegister16(_iAddr, _mxtdata.t46_cte_config_address, rb, 4);
            ets_printf("mxt T46 readback: CTRL=0x%02X MODE=%d IDLESYNCSPERX=%d ACTSYNCSPERX=%d\r\n", rb[0], rb[1], rb[2], rb[3]);
        }

        // T56 Shieldless: reference config (Table 11-1) enables T56 with INTTIME=26.
        // Write only the first 2 fields (CTRL + INTTIME) to avoid overwriting adjacent objects.
        if (_mxtdata.t56_shieldless_address) {
            uint8_t t56wr[2 + 2] = {};
            t56wr[0] = (uint8_t)(_mxtdata.t56_shieldless_address);
            t56wr[1] = (uint8_t)(_mxtdata.t56_shieldless_address >> 8);
            t56wr[2 + 0] = 0x01;  // CTRL: ENABLE
            t56wr[2 + 1] = 26;    // INTTIME
            I2CWrite(_iAddr, t56wr, 2 + 2);
        }

        // T100: multitouch touchscreen. XSIZE/YSIZE must match the sensor matrix.
        // Object table size field is (actual-1), so actual T100 size = 68 bytes (offsets 0-67).
        // TCHEVENTCFG offset is uncertain across MXT variants (offset 4 vs 5) — set both.
        // T100 layout: [0]=CTRL [4/5]=TCHEVENTCFG [7]=NUMTCH
        //   [10]=XSIZE [14-15]=XRANGE [21]=YSIZE [25-26]=YRANGE [33]=TCHTHR [34]=TCHHYST
        {
            uint8_t t100wr[2 + 68] = {};
            t100wr[0] = (uint8_t)(_mxtdata.t100_multiple_touch_touchscreen_address);
            t100wr[1] = (uint8_t)(_mxtdata.t100_multiple_touch_touchscreen_address >> 8);
            t100wr[2 +  0] = 0x07;  // CTRL: ENABLE | RPTEN | SCANEN
            t100wr[2 +  4] = 0xFF;  // TCHEVENTCFG (offset 4, all events)
            t100wr[2 +  5] = 0xFF;  // TCHEVENTCFG (offset 5, all events — covers both placements)
            t100wr[2 +  7] = 5;     // NUMTCH
            t100wr[2 + 10] = _mxtdata.matrix_x_size;  // XSIZE
            t100wr[2 + 14] = 0xFF;  // XRANGE lo
            t100wr[2 + 15] = 0x0F;  // XRANGE hi
            t100wr[2 + 21] = _mxtdata.matrix_y_size;  // YSIZE
            t100wr[2 + 25] = 0xFF;  // YRANGE lo
            t100wr[2 + 26] = 0x0F;  // YRANGE hi
            t100wr[2 + 33] = 5;     // TCHTHR
            t100wr[2 + 34] = 2;     // TCHHYST
            I2CWrite(_iAddr, t100wr, 2 + 68);

        }
        // BACKUP (T6[1] = 0x55): save RAM config → NVM to clear CFGERR.
        // Per datasheet §9.3, chip auto-resets after BACKUP — must read the RESET
        // message from T5 before issuing any further commands.
        {
            uint8_t t5_size = _mxtdata.t5_max_message_size;
            uint8_t msg[16] = {};

            uint16_t backup_reg = _mxtdata.t6_command_processor_address + 1;
            uint8_t backup_cmd[3] = {(uint8_t)backup_reg, (uint8_t)(backup_reg >> 8), 0x55};
            I2CWrite(_iAddr, backup_cmd, 3);
            ets_printf("mxt: BACKUP issued, waiting for auto-reset...\r\n");

            bool reset_seen = false;
            for (int ms = 0; ms < 2000 && !reset_seen; ms += 10) {
                vTaskDelay(pdMS_TO_TICKS(10));
                uint8_t count = 0;
                I2CReadRegister16(_iAddr, _mxtdata.t44_message_count_address, &count, 1);
                for (int m = 0; m < count; m++) {
                    memset(msg, 0, sizeof(msg));
                    I2CReadRegister16(_iAddr, _mxtdata.t5_message_processor_address, msg, t5_size);
                    ets_printf("mxt: BACKUP msg @%dms rid=%d status=0x%02X\r\n", ms, msg[0], msg[1]);
                    if (msg[1] & 0x80) {  // RESET bit — BACKUP complete
                        reset_seen = true;
                    }
                }
            }
            if (!reset_seen) {
                ets_printf("mxt: BACKUP timed out — no RESET message received\r\n");
            }
        }

        // CALIBRATE (T6[2] = 0x01): establish fresh baseline after BACKUP.
        // Poll T5 messages to confirm CAL completes (CAL bit set then cleared).
        {
            uint8_t t5_size = _mxtdata.t5_max_message_size;
            uint8_t msg[16] = {};

            uint16_t cal_reg = _mxtdata.t6_command_processor_address + 2;
            uint8_t cal_cmd[3] = {(uint8_t)cal_reg, (uint8_t)(cal_reg >> 8), 0x01};
            I2CWrite(_iAddr, cal_cmd, 3);
            ets_printf("mxt: CALIBRATE issued\r\n");

            bool cal_done = false;
            for (int ms = 0; ms < 2000 && !cal_done; ms += 10) {
                vTaskDelay(pdMS_TO_TICKS(10));
                uint8_t count = 0;
                I2CReadRegister16(_iAddr, _mxtdata.t44_message_count_address, &count, 1);
                for (int m = 0; m < count; m++) {
                    memset(msg, 0, sizeof(msg));
                    I2CReadRegister16(_iAddr, _mxtdata.t5_message_processor_address, msg, t5_size);
                    ets_printf("mxt: CAL msg @%dms rid=%d status=0x%02X\r\n", ms, msg[0], msg[1]);
                    // CAL complete: T6 message (rid=1) with CAL bit (0x10) clear
                    if (msg[0] == 1 && !(msg[1] & 0x10) && !(msg[1] & 0x80)) {
                        ets_printf("mxt: CALIBRATE complete, STATUS=0x%02X\r\n", msg[1]);
                        cal_done = true;
                    }
                }
            }
            if (!cal_done) {
                ets_printf("mxt: CALIBRATE timed out\r\n");
            }
        }
    }
    return rc;
}

//
// Read touch samples from the MXT144
//
int BBCapTouch::getSamples(TOUCHINFO *pTI) {
    uint8_t ucTemp[32];
    int i, j;

    if (!pTI) return 0;
    pTI->count = 0;

    if (!_mxtdata.t44_message_count_address) {
        ets_printf("mxt: no t44 addr\r\n");
        return 0;
    }
    I2CReadRegister16(_iAddr, _mxtdata.t44_message_count_address, ucTemp, 1);
    j = ucTemp[0];
    if (j > 0) ets_printf("mxt: msg_count=%d t100_frid=%d\r\n", j, _mxtdata.t100_first_report_id);
    uint8_t t5_size = (_mxtdata.t5_max_message_size && _mxtdata.t5_max_message_size <= 32)
                      ? _mxtdata.t5_max_message_size : MXT_MESSAGE_SIZE;
    for (i = 0; i < j; i++) {
        I2CReadRegister16(_iAddr, _mxtdata.t5_message_processor_address, ucTemp, t5_size);
        ets_printf("mxt: rid=%d b1=0x%02X x=%d y=%d\r\n", ucTemp[0], ucTemp[1],
                   ucTemp[2]|(ucTemp[3]<<8), ucTemp[4]|(ucTemp[5]<<8));
        // T100: RID+0 = screen status, RID+1..+N = finger 0..N-1
        if (ucTemp[0] >= _mxtdata.t100_first_report_id + 1 &&
            ucTemp[0] <  _mxtdata.t100_first_report_id + 1 + 5) {
            uint8_t finger_idx = ucTemp[0] - _mxtdata.t100_first_report_id - 1;
            uint8_t event = ucTemp[1] & 0xf;
            if (finger_idx + 1 > (uint8_t)pTI->count) pTI->count = finger_idx + 1;
            pTI->x[finger_idx] = ucTemp[2] + (ucTemp[3] << 8);
            pTI->y[finger_idx] = ucTemp[4] + (ucTemp[5] << 8);
            if (event == 1 || event == 4) {       // press / move
                pTI->area[finger_idx] = 50;
            } else if (event == 5) {              // release
                pTI->area[finger_idx] = 0;
            }
        }
    }
    return (pTI->count > 0);
}

//
// Read the T37 diagnostic object and print a raw delta matrix.
// Command 0x10 = DELTA_DATA: shows capacitance change vs baseline.
// Values change when a finger is present; all-zero means the sensor is not sensing.
//
void BBCapTouch::dumpDiagnostic(void) {
    if (!_mxtdata.t37_diagnostic_address || !_mxtdata.t6_command_processor_address) {
        ets_printf("mxt diag: T37 or T6 address not found in object table\r\n");
        return;
    }
    int cols = _mxtdata.matrix_x_size ? _mxtdata.matrix_x_size : 12;
    int rows = _mxtdata.matrix_y_size ? _mxtdata.matrix_y_size : 12;

    uint16_t diag_reg = _mxtdata.t6_command_processor_address + 5;
    uint8_t cmd[3];
    cmd[0] = (uint8_t)diag_reg;
    cmd[1] = (uint8_t)(diag_reg >> 8);
    uint8_t buf[2 + 128] = {};

    // --- Reference data (0x11): uint16 per node, 2 bytes each.
    // Page 0 = 64 nodes. Non-zero here means CTE is running and has a baseline.
    cmd[2] = 0x11;
    I2CWrite(_iAddr, cmd, 3);
    vTaskDelay(pdMS_TO_TICKS(20));
    I2CReadRegister16(_iAddr, _mxtdata.t37_diagnostic_address, buf, 2 + 128);
    ets_printf("mxt T37 mode=0x%02X page=%d (reference, uint16, first %d nodes):\r\n",
               buf[0], buf[1], 64);
    for (int i = 0; i < 64; i++) {
        uint16_t ref = buf[2 + i * 2] | (buf[2 + i * 2 + 1] << 8);
        ets_printf("%6u", ref);
        if ((i + 1) % cols == 0) ets_printf("\r\n");
    }

    // --- Delta data (0x10): int8 per node.
    // Page 0 = 128 nodes. Non-zero here means a finger is changing capacitance.
    cmd[2] = 0x10;
    I2CWrite(_iAddr, cmd, 3);
    vTaskDelay(pdMS_TO_TICKS(20));
    memset(buf, 0, sizeof(buf));
    I2CReadRegister16(_iAddr, _mxtdata.t37_diagnostic_address, buf, 2 + 128);
    ets_printf("mxt T37 mode=0x%02X page=%d (delta, int8, first %d nodes):\r\n",
               buf[0], buf[1], rows * cols < 128 ? rows * cols : 128);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r * cols + c;
            if (idx >= 128) break;
            ets_printf("%4d", (int8_t)buf[2 + idx]);
        }
        ets_printf("\r\n");
    }
}

bool BBCapTouch::I2CTest(uint8_t u8Addr) {
    uint8_t c;
    return (i2c_master_read_from_device(I2C_NUM_0, u8Addr, &c, 1, pdMS_TO_TICKS(10)) == ESP_OK);
}

int BBCapTouch::I2CWrite(uint8_t u8Addr, uint8_t *pData, int iLen) {
    return (i2c_master_write_to_device(I2C_NUM_0, u8Addr, pData, iLen, pdMS_TO_TICKS(100)) == ESP_OK);
}

int BBCapTouch::I2CReadRegister16(uint8_t u8Addr, uint16_t u16Register, uint8_t *pData, int iLen) {
    uint8_t ucTemp[2];
    ucTemp[0] = (uint8_t)u16Register;        // low byte (MXT144 is little-endian)
    ucTemp[1] = (uint8_t)(u16Register >> 8); // high byte
    i2c_master_write_read_device(I2C_NUM_0, u8Addr, ucTemp, 2, pData, iLen, pdMS_TO_TICKS(100));
    return iLen;
}

int BBCapTouch::I2CRead(uint8_t u8Addr, uint8_t *pData, int iLen) {
    int rc = i2c_master_read_from_device(I2C_NUM_0, u8Addr, pData, iLen, pdMS_TO_TICKS(100));
    return (rc == ESP_OK) ? iLen : 0;
}
