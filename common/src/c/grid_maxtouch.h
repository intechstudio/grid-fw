#ifndef GRID_MAXTOUCH_H
#define GRID_MAXTOUCH_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "grid_ui_touch.h"
#include "grid_utask.h"

#define MXT_I2C_ADDR 0x4A

#define MXT_CFG_MAGIC "OBP_RAW V4\r\n"
#define MXT_CFG_ENCRYPTION "ENCRYPTION 0\r\nMAX_ENCRYPTION_BLOCKS 0\r\n"
#define MXT_CFG_NO_DEVICES "NO_DEVICES 1\r\n"
#define MXT_CFG_DEVICE_0 "DEVICE_0\r\n"

#define MXT_OBJECT_START 0x07
#define MXT_OBJECT_SIZE 6
#define MXT_INFO_CHECKSUM_SIZE 3
#define MXT_MAX_BLOCK_WRITE 256

#define MXT_DEBUG_DIAGNOSTIC_T37 37
#define MXT_GEN_MESSAGE_T5 5
#define MXT_GEN_COMMAND_T6 6
#define MXT_GEN_POWER_T7 7
#define MXT_SPT_MESSAGECOUNT_T44 44
#define MXT_GEN_DYNAMICCONFIGURATIONCONTAINER_T71 71
#define MXT_TOUCH_MULTITOUCHSCREEN_T100 100

#define MXT_RPTID_NOMSG 0xff

#define MXT_COMMAND_RESET 0
#define MXT_COMMAND_BACKUPNV 1
#define MXT_COMMAND_CALIBRATE 2
#define MXT_COMMAND_REPORTALL 3
#define MXT_COMMAND_DIAGNOSTIC 5

#define MXT_T6_STATUS_RESET BIT(7)
#define MXT_T6_STATUS_OFL BIT(6)
#define MXT_T6_STATUS_SIGERR BIT(5)
#define MXT_T6_STATUS_CAL BIT(4)
#define MXT_T6_STATUS_CFGERR BIT(3)
#define MXT_T6_STATUS_COMSERR BIT(2)

#define MXT_RESET_VALUE 0x01
#define MXT_BACKUP_VALUE 0x55

#define MXT_T100_DETECT BIT(7)
#define MXT_T100_TYPE_MASK 0x70
#define MXT_T100_EVENT_MASK 0x0f

#define MXT_DIAGNOSTIC_PAGEUP 0x01
#define MXT_DIAGNOSTIC_DELTAS 0x10
#define MXT_DIAGNOSTIC_REFS 0x11

enum t100_type {
  MXT_T100_TYPE_FINGER = 1,
  MXT_T100_TYPE_PASSIVE_STYLUS = 2,
  MXT_T100_TYPE_HOVERING_FINGER = 4,
  MXT_T100_TYPE_GLOVE = 5,
  MXT_T100_TYPE_LARGE_TOUCH = 6,
};

struct __attribute__((__packed__)) mxt_object {
  uint8_t type;
  uint16_t start_addr;
  uint8_t size_minus_one;
  uint8_t inst_minus_one;
  uint8_t num_report_ids;
};

struct __attribute__((__packed__)) mxt_info {
  uint8_t family_id;
  uint8_t variant_id;
  uint8_t version;
  uint8_t build;
  uint8_t matrix_xsize;
  uint8_t matrix_ysize;
  uint8_t object_num;
};

#define MXT_XLINES 12
#define MXT_YLINES 12
#define MXT_XYNODES (MXT_XLINES * MXT_YLINES)
#define MXT_DIAGNOSTIC_SIZE 128

struct t37_debug {
  uint8_t mode;
  uint8_t page;
  uint8_t data[MXT_DIAGNOSTIC_SIZE];
};

// Minimum number of pages to accommodate reference and delta debug modes
#define MXT_DBG_PAGES_MIN (((MXT_XYNODES * 2) / MXT_DIAGNOSTIC_SIZE) + 1)

struct mxt_dbg {
  uint16_t T37_address;
  uint16_t diag_cmd_address;
  struct t37_debug t37_buf[MXT_DBG_PAGES_MIN];
  unsigned int t37_pages;
  unsigned int t37_nodes;
};

struct mxt_cfg {
  uint8_t* raw;
  size_t raw_size;
  size_t raw_pos;
  struct mxt_info info;
  size_t start_ofs;
  uint8_t* mem;
  uint16_t mem_size;
};

struct firmware {
  size_t size;
  const uint8_t* data;
};

struct mxt_data {

  uint8_t* raw_info_block;

  struct mxt_info info;
  uint32_t info_crc;

  struct mxt_object* object_table;

  uint16_t mem_size;

  uint32_t config_crc;
  uint8_t crc_completion;

  uint8_t reset_completion;

  uint8_t max_reportid;

  uint16_t T5_address;
  uint8_t T5_msg_size;
  uint8_t T6_reportid;
  uint16_t T6_address;
  uint16_t T7_address;
  uint16_t T44_address;
  uint16_t T71_address;
  uint8_t T100_rid_min;
  uint8_t T100_rid_max;

  uint8_t T6_status;

  struct mxt_dbg dbg;
};

typedef void (*grid_process_touch_t)(struct touchinfo_t*);

struct grid_maxtouch_model {

  grid_process_touch_t process_touch;
};

extern struct grid_maxtouch_model grid_maxtouch_state;

void grid_maxtouch_init(struct grid_maxtouch_model* maxtouch, grid_process_touch_t process_touch);

uint32_t grid_maxtouch_calculate_crc(uint8_t* base, off_t start_off, off_t end_off);

size_t maxtouch_obj_size(const struct mxt_object* obj);

size_t maxtouch_obj_instances(const struct mxt_object* obj);

struct mxt_object* maxtouch_get_object(struct mxt_data* data, uint8_t type);

bool grid_maxtouch_read_info_block(struct mxt_data* data);

void grid_maxtouch_process_msgs(struct mxt_data* data, uint8_t* msg_buf);

void grid_maxtouch_debug_init(struct mxt_data* data);

void grid_maxtouch_utask_t37(struct grid_utask_timer* timer, struct mxt_data* data);

int grid_maxtouch_update_crc(struct mxt_data* data, uint8_t cmd, uint8_t val);

int grid_maxtouch_soft_reset(struct mxt_data* data);

int grid_maxtouch_update(struct mxt_data* data);

#endif /* GRID_MAXTOUCH_H */
