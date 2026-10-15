#include "grid_maxtouch.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "grid_math.h"
#include "grid_platform.h"

#include "mxt144u_cfg.h"

GRID_DRAM_ATTR struct grid_maxtouch_model grid_maxtouch_state;

void grid_maxtouch_init(struct grid_maxtouch_model* maxtouch, grid_process_touch_t process_touch) {

  assert(process_touch);

  maxtouch->process_touch = process_touch;
}

static void mxt_calc_crc24(uint32_t* crc, uint8_t firstbyte, uint8_t secondbyte) {

  static const unsigned int crcpoly = 0x80001B;
  uint32_t result;
  uint32_t data_word;

  data_word = (secondbyte << 8) | firstbyte;
  result = ((*crc << 1) ^ data_word);

  if (result & 0x1000000) {
    result ^= crcpoly;
  }

  *crc = result;
}

uint32_t grid_maxtouch_calculate_crc(uint8_t* base, off_t start_off, off_t end_off) {

  uint32_t crc = 0;
  uint8_t* ptr = base + start_off;
  uint8_t* last_val = base + end_off - 1;

  if (end_off < start_off) {
    return -EINVAL;
  }

  while (ptr < last_val) {
    mxt_calc_crc24(&crc, *ptr, *(ptr + 1));
    ptr += 2;
  }

  if (ptr == last_val) {
    mxt_calc_crc24(&crc, *ptr, 0);
  }

  crc &= 0x00FFFFFF;

  return crc;
}

size_t maxtouch_obj_size(const struct mxt_object* obj) { return obj->size_minus_one + 1; }

size_t maxtouch_obj_instances(const struct mxt_object* obj) { return obj->inst_minus_one + 1; }

struct mxt_object* maxtouch_get_object(struct mxt_data* data, uint8_t type) {

  for (int i = 0; i < data->info.object_num; ++i) {

    struct mxt_object* object = &data->object_table[i];

    if (object->type == type) {
      return object;
    }
  }

  grid_platform_printf("mxt_get_object: invalid type T%hhu\n", type);
  return NULL;
}

void grid_maxtouch_parse_info_block(struct mxt_data* data, const uint8_t* iblk) {

  struct mxt_info* info = (struct mxt_info*)iblk;

  // Parse object table
  uint8_t reportid = 1;
  data->mem_size = 0;
  uint8_t min_id = 0;
  uint8_t max_id = 0;
  for (uint8_t i = 0; i < info->object_num; ++i) {

    struct mxt_object* object = (struct mxt_object*)&iblk[MXT_OBJECT_START + i * MXT_OBJECT_SIZE];

    if (object->num_report_ids) {

      min_id = reportid;
      reportid += object->num_report_ids * maxtouch_obj_instances(object);
      max_id = reportid - 1;

    } else {

      min_id = max_id = 0;
    }

    switch (object->type) {
    case MXT_GEN_MESSAGE_T5: {

      data->T5_address = object->start_addr;

      if (info->family_id == 0x80 && info->version < 0x20) {
        data->T5_msg_size = maxtouch_obj_size(object);
      } else {
        data->T5_msg_size = maxtouch_obj_size(object) - 1;
      }

    } break;
    case MXT_GEN_COMMAND_T6: {

      data->T6_address = object->start_addr;
      data->T6_reportid = min_id;

    } break;
    case MXT_GEN_POWER_T7: {

      data->T7_address = object->start_addr;

    } break;
    case MXT_SPT_MESSAGECOUNT_T44: {

      data->T44_address = object->start_addr;

    } break;
    case MXT_GEN_DYNAMICCONFIGURATIONCONTAINER_T71: {

      data->T71_address = object->start_addr;

    } break;
    case MXT_TOUCH_MULTITOUCHSCREEN_T100: {

      data->T100_rid_min = min_id;
      data->T100_rid_max = max_id;

    } break;
    }

    // Update the maximum memory size based on the address following the object
    uint16_t addr_after = object->start_addr + maxtouch_obj_size(object) * maxtouch_obj_instances(object);
    if (addr_after >= data->mem_size) {
      data->mem_size = addr_after;
    }
  }

  // Store maximum reportid
  data->max_reportid = reportid;
}

bool grid_maxtouch_read_info_block(struct mxt_data* data) {

  // Read ID information block
  struct mxt_info* info = &data->info;
  if (grid_platform_mxt_read(0, sizeof(struct mxt_info), (uint8_t*)info)) {
    grid_platform_printf("failed to read first 7 bytes\n");
    return false;
  }

  // Allocate information block
  size_t info_size = MXT_OBJECT_START + sizeof(struct mxt_object) * info->object_num + MXT_INFO_CHECKSUM_SIZE;
  uint8_t* iblk = data->raw_info_block = grid_platform_allocate_volatile(info_size);
  if (!iblk) {
    grid_platform_printf("failed allocate info block\n");
    return false;
  }

  // Read information block
  if (grid_platform_mxt_read(0, info_size, iblk)) {
    grid_platform_printf("failed to read info block\n");
    return false;
  }

  // Extract checksum
  uint8_t* crc_ptr = &iblk[info_size - MXT_INFO_CHECKSUM_SIZE];
  uint32_t info_crc = crc_ptr[0] | crc_ptr[1] << 8 | crc_ptr[2] << 16;
  uint32_t calc_crc = grid_maxtouch_calculate_crc(iblk, 0, info_size - MXT_INFO_CHECKSUM_SIZE);
  if (info_crc == 0 || info_crc != calc_crc) {
    grid_platform_printf("crc mismatch\n");
    return false;
  }
  data->info_crc = info_crc;

  grid_platform_printf("family %02X variant %02X firmware v%u.%u.%02X objects %u\n", info->family_id, info->variant_id, info->version >> 4, info->version & 0xf, info->build, info->object_num);

  // Parse information block
  grid_maxtouch_parse_info_block(data, iblk);

  data->object_table = (struct mxt_object*)&data->raw_info_block[MXT_OBJECT_START];

  return true;
}

#define BIT(n) (1UL << (n))

void grid_maxtouch_proc_t6(struct mxt_data* data, uint8_t* msg) {

  uint8_t status = msg[1];
  uint32_t crc = msg[2] | (msg[3] << 8) | (msg[4] << 16);

  if (crc != data->config_crc) {
    data->config_crc = crc;
  }

  data->crc_completion = 1;

  if (status & MXT_T6_STATUS_RESET) {
    data->reset_completion = 1;
  }

  if (status != data->T6_status) {

    grid_platform_printf("T6 status 0x%02X%s%s%s%s%s%s%s\n", status, status == 0 ? " OK" : "", status & MXT_T6_STATUS_RESET ? " RESET" : "", status & MXT_T6_STATUS_OFL ? " OFL" : "",
                         status & MXT_T6_STATUS_SIGERR ? " SIGERR" : "", status & MXT_T6_STATUS_CAL ? " CAL" : "", status & MXT_T6_STATUS_CFGERR ? " CFGERR" : "",
                         status & MXT_T6_STATUS_COMSERR ? " COMSERR" : "");
  }

  data->T6_status = status;
}

void grid_maxtouch_proc_t100(struct mxt_data* data, uint8_t* msg) {

  int id = msg[0] - data->T100_rid_min - 2;

  // Ignore SCRSTATUS event
  if (id < 0) {
    return;
  }

  uint8_t status = msg[1];
  uint16_t x = msg[2] + (msg[3] << 8);
  uint16_t y = msg[4] + (msg[5] << 8);
  uint8_t distance = 0;

  uint8_t type = (status & MXT_T100_TYPE_MASK) >> 4;

  switch (type) {
  case MXT_T100_TYPE_FINGER:
  case MXT_T100_TYPE_PASSIVE_STYLUS:
  case MXT_T100_TYPE_HOVERING_FINGER:
  case MXT_T100_TYPE_GLOVE: {

    struct touchinfo_t info = (struct touchinfo_t){
        .id = id,
        .event = status & MXT_T100_EVENT_MASK,
        .x = x,
        .y = y,
    };
    grid_maxtouch_state.process_touch(&info);

  } break;
  case MXT_T100_TYPE_LARGE_TOUCH:
    break;
  default:
    return;
  }
}

void grid_maxtouch_proc_message(struct mxt_data* data, uint8_t* msg) {

  uint8_t report_id = msg[0];

  if (report_id == MXT_RPTID_NOMSG) {
    return;
  }

  if (report_id == data->T6_reportid) {
    grid_maxtouch_proc_t6(data, msg);
  } else if (report_id >= data->T100_rid_min && report_id <= data->T100_rid_max) {
    grid_maxtouch_proc_t100(data, msg);
  }
}

void grid_maxtouch_process_msgs(struct mxt_data* data) {

  // Read the number of messages from T44
  uint8_t count;
  if (grid_platform_mxt_read(data->T44_address, 1, &count)) {
    return;
  }

  if (!count) {
    return;
  }

  // Read messages from T5
  if (grid_platform_mxt_read(data->T5_address, data->T5_msg_size * count, data->msg_buf)) {
    return;
  }

  // Process messages
  for (uint8_t i = 0; i < count; ++i) {
    grid_maxtouch_proc_message(data, data->msg_buf + i * data->T5_msg_size);
  }

  return;
}

void grid_maxtouch_debug_init(struct mxt_data* data) {

  struct mxt_info* info = &data->info;
  struct mxt_dbg* dbg = &data->dbg;
  struct mxt_object* object;

  object = maxtouch_get_object(data, MXT_GEN_COMMAND_T6);
  if (!object) {
    goto mxt_debug_init_error;
  }

  dbg->diag_cmd_address = object->start_addr + MXT_COMMAND_DIAGNOSTIC;

  object = maxtouch_get_object(data, MXT_DEBUG_DIAGNOSTIC_T37);
  if (!object) {
    goto mxt_debug_init_error;
  }

  if (maxtouch_obj_size(object) != sizeof(struct t37_debug)) {
    grid_platform_printf("bad t37 size (expected %d, got %d)\n", sizeof(struct t37_debug), maxtouch_obj_size(object));
    goto mxt_debug_init_error;
  }

  dbg->T37_address = object->start_addr;

  dbg->t37_nodes = MXT_XYNODES;

  dbg->t37_pages = (dbg->t37_nodes * sizeof(uint16_t)) / sizeof(dbg->t37_buf->data) + 1;

  if (dbg->t37_pages != MXT_DBG_PAGES_MIN) {
    grid_platform_printf("wrong number of allocated debug pages\n");
    goto mxt_debug_init_error;
  }

  return;

mxt_debug_init_error:
  grid_platform_printf("failed to initialize t37\n");
}

int grid_maxtouch_read_diagnostic_debug(struct mxt_data* data, uint8_t mode) {

  struct mxt_dbg* dbg = &data->dbg;
  int retries = 0;
  uint8_t cmd = mode;
  uint8_t cmd_poll;

  for (int page = 0; page < MXT_DBG_PAGES_MIN; ++page) {

    struct t37_debug* p = &dbg->t37_buf[0] + page;

    if (grid_platform_mxt_write(dbg->diag_cmd_address, 1, &cmd)) {
      return 1;
    }

    int timeout_counter = 0;

    do {

      grid_platform_delay_ms(10);

      if (grid_platform_mxt_read(dbg->diag_cmd_address, 1, &cmd_poll)) {
        return 1;
      }

    } while (cmd_poll && timeout_counter++ <= 100);

    if (timeout_counter > 100) {
      return 1;
    }

    if (grid_platform_mxt_read(dbg->T37_address, sizeof(struct t37_debug), (uint8_t*)p)) {
      return 1;
    }

    if (p->mode != mode || p->page != page) {
      grid_platform_printf("T37 page mismatch\n");
      return 1;
    }

    // For remaining pages, write PAGEUP rather than mode
    cmd = MXT_DIAGNOSTIC_PAGEUP;
  }

  return 0;
}

void grid_maxtouch_display_debug_pages(struct mxt_dbg* dbg) {

  assert(MXT_XLINES == MXT_YLINES);

  int values = 0;

  for (int page = 0; page < MXT_DBG_PAGES_MIN; ++page) {

    struct t37_debug* p = &dbg->t37_buf[0] + page;

    for (int j = 0; j < MXT_DIAGNOSTIC_SIZE; j += sizeof(uint16_t)) {

      int16_t val = p->data[j + 1] << 8 | p->data[j];
      grid_platform_printf("%s%5d ", (val > 0) - (val < 0) ? "+" : "-", abs(val));
      grid_platform_delay_us(50);

      if (++values >= dbg->t37_nodes) {
        break;
      }

      if (values % MXT_XLINES == 0) {
        grid_platform_printf("\n");
      }
    }
  }

  grid_platform_printf("\n");
}

void grid_maxtouch_utask_t37(struct grid_utask_timer* timer, struct mxt_data* data) {

  if (!grid_utask_timer_elapsed(timer)) {
    return;
  }

  if (grid_maxtouch_read_diagnostic_debug(data, MXT_DIAGNOSTIC_REFS)) {
    grid_platform_printf("mxt_read_diagnostic_debug failed\n");
  }

  grid_maxtouch_display_debug_pages(&data->dbg);
  grid_platform_printf("\n");
}

static int maxtouch_t6_command(struct mxt_data* data, uint8_t cmd_off, uint8_t val, bool wait) {

  int ret;

  uint16_t reg = data->T6_address + cmd_off;

  if ((ret = grid_platform_mxt_write(reg, 1, &val))) {
    return ret;
  }

  if (!wait) {
    return 0;
  }

  uint8_t cmd_reg;
  int timeout_counter = 0;

  do {

    grid_platform_delay_ms(10);

    if ((ret = grid_platform_mxt_read(reg, 1, &cmd_reg))) {
      return ret;
    }

  } while (cmd_reg && timeout_counter++ <= 100);

  if (timeout_counter > 100) {
    return -1;
  }

  return 0;
}

int grid_maxtouch_update_crc(struct mxt_data* data, uint8_t cmd, uint8_t val) {

  data->config_crc = 0;
  data->crc_completion = 0;

  int ret;

  if ((ret = maxtouch_t6_command(data, cmd, val, true))) {
    return ret;
  }

  while (!data->crc_completion) {
    grid_maxtouch_process_msgs(data);
  }

  return 0;
}

int grid_maxtouch_soft_reset(struct mxt_data* data) {

  data->reset_completion = 0;

  int ret;

  if ((ret = maxtouch_t6_command(data, MXT_COMMAND_RESET, MXT_RESET_VALUE, false))) {
    return ret;
  }

  grid_platform_delay_ms(100);

  while (!data->reset_completion) {
    grid_maxtouch_process_msgs(data);
  }

  return 0;
}

#define hex_to_uint8_range(h, min, max, add) (((h) >= (min) && (h) <= (max)) * ((h) - (min) + (add)))

int sscanf_u8_space(const char* str, uint8_t* dest, int* offset) {

  uint8_t hi = hex_to_uint8_range(str[0], '0', '9', 0) + hex_to_uint8_range(str[0], 'A', 'F', 10);

  if (hi == 0 && str[0] != '0') {
    return 0;
  }

  uint8_t lo = hex_to_uint8_range(str[1], '0', '9', 0) + hex_to_uint8_range(str[1], 'A', 'F', 10);

  if (lo == 0 && str[1] != '0') {
    return 0;
  }

  if (str[2] != ' ' && str[2] != '\n') {
    return 0;
  }

  *dest = (hi << 4) + lo;
  *offset = 3;
  return 1;
}

int maxtouch_prepare_cfg_mem(struct mxt_data* data, struct mxt_cfg* cfg) {

  int ret;

  unsigned int type, inst, size;
  int offset;
  while (cfg->raw_pos < cfg->raw_size) {

    // Read type, instance, length
    ret = sscanf((char*)&cfg->raw[cfg->raw_pos], "%x %x %x %n", &type, &inst, &size, &offset);

    if (ret == 0) {
      // EOF
      break;
    } else if (ret != 3) {
      grid_platform_printf("mxt_prepare_cfg_mem: bad format\n");
      return -EINVAL;
    }
    cfg->raw_pos += offset;

    struct mxt_object* object = maxtouch_get_object(data, type);
    if (!object) {

      // Skip object
      uint8_t val;
      for (int i = 0; i < size; ++i) {
        ret = sscanf_u8_space((char*)&cfg->raw[cfg->raw_pos], &val, &offset);
        if (ret != 1) {
          grid_platform_printf("mxt_prepare_cfg_mem: bad format in T%d at %d\n", type, i);
          return -EINVAL;
        }
        cfg->raw_pos += offset;
      }
      continue;
    }

    if (size > maxtouch_obj_size(object)) {

      // Either we are in fallback mode due to a wrong config,
      // or this is a configuration from a later fw version,
      // or this is a corrupt or hand-edited configuration.
      grid_platform_printf("mxt_prepare_cfg_mem: discarding %u byte(s) in T%d\n", size - maxtouch_obj_size(object), type);

    } else if (maxtouch_obj_size(object) > size) {

      // An upgraded firmware may add new bytes to the end of objects.
      // It is generally forward compatible to zero these bytes, and
      // previous behaviour will be retained. However, this does invalidate
      // the CRC and will force fallback mode until the config is updated.
      grid_platform_printf("mxt_prepare_cfg_mem: zeroing %u byte(s) in T%d\n", maxtouch_obj_size(object) - size, type);
    }

    if (inst >= maxtouch_obj_instances(object)) {
      grid_platform_printf("mxt_prepare_cfg_mem: too many instances\n");
      return -EINVAL;
    }

    uint16_t reg = object->start_addr + maxtouch_obj_size(object) * inst;

    uint8_t val;
    for (int i = 0; i < size; ++i) {

      ret = sscanf_u8_space((char*)&cfg->raw[cfg->raw_pos], &val, &offset);
      if (ret != 1) {
        grid_platform_printf("mxt_prepare_cfg_mem: bad format in T%d at %d\n", type, i);
        return -EINVAL;
      }
      cfg->raw_pos += offset;

      if (i >= maxtouch_obj_size(object)) {
        continue;
      }

      unsigned int byte_off = reg + i - cfg->start_ofs;

      if (byte_off >= cfg->mem_size) {
        grid_platform_printf("mxt_prepare_cfg_mem: bad object (reg %d) in T%d at %d\n", reg, object->type, byte_off);
        return -EINVAL;
      }

      *(cfg->mem + byte_off) = val;
    }
  }

  return 0;
}

int maxtouch_upload_cfg_mem(struct mxt_data* data, struct mxt_cfg* cfg) {

  for (size_t off = 0; off < cfg->mem_size; off += MXT_MAX_BLOCK_WRITE) {

    uint16_t size = MIN(cfg->mem_size - off, MXT_MAX_BLOCK_WRITE);

    int ret;

    if ((ret = grid_platform_mxt_write(cfg->start_ofs + off, size, cfg->mem + off))) {
      return ret;
    }
  }

  return 0;
}

int maxtouch_update_cfg(struct mxt_data* data, struct firmware* fw) {

  int ret = 0;

  grid_maxtouch_update_crc(data, MXT_COMMAND_REPORTALL, 1);

  // Initialize config update context
  struct mxt_cfg cfg = (struct mxt_cfg){.raw_size = fw->size};

  // Allocate zero terminated copy of the file
  cfg.raw = malloc(cfg.raw_size + 1);
  if (!cfg.raw) {
    goto mxt_update_cfg_error;
  }
  memcpy(cfg.raw, fw->data, cfg.raw_size);
  cfg.raw[cfg.raw_size] = '\0';

  // Parse magic bytes
  if (strncmp((char*)&cfg.raw[cfg.raw_pos], MXT_CFG_MAGIC, strlen(MXT_CFG_MAGIC))) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += strlen(MXT_CFG_MAGIC);

  // Parse encryption configuration
  if (strncmp((char*)&cfg.raw[cfg.raw_pos], MXT_CFG_ENCRYPTION, strlen(MXT_CFG_ENCRYPTION))) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += strlen(MXT_CFG_ENCRYPTION);

  // Parse number of devices, expecting only one
  if (strncmp((char*)&cfg.raw[cfg.raw_pos], MXT_CFG_NO_DEVICES, strlen(MXT_CFG_NO_DEVICES))) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += strlen(MXT_CFG_NO_DEVICES);

  // Parse information block
  int offset;
  for (size_t i = 0; i < sizeof(struct mxt_info); ++i) {

    if (sscanf_u8_space((char*)&cfg.raw[cfg.raw_pos], (uint8_t*)&cfg.info + i, &offset) != 1) {
      goto mxt_update_cfg_error;
    }
    cfg.raw_pos += offset;
  }

  if (cfg.info.family_id != data->info.family_id) {
    grid_platform_printf("family ID mismatch\n");
    goto mxt_update_cfg_error;
  }

  if (cfg.info.variant_id != data->info.variant_id) {
    grid_platform_printf("variant ID mismatch\n");
    goto mxt_update_cfg_error;
  }

  // Parse info CRC
  uint32_t info_crc;
  if (sscanf((char*)&cfg.raw[cfg.raw_pos], "%x\r\n%n", (unsigned int*)&info_crc, &offset) != 1) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += offset;

  // Parse config CRC
  uint32_t config_crc;
  if (sscanf((char*)&cfg.raw[cfg.raw_pos], "%x\r\n%n", (unsigned int*)&config_crc, &offset) != 1) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += offset;

  // Check for CRC mismatches
  if (info_crc == data->info_crc) {

    if (config_crc == 0 || data->config_crc == 0) {

      grid_platform_printf("config CRC zero, attempting to apply config\n");

    } else if (config_crc == data->config_crc) {

      grid_platform_printf("config CRC %06X OK\n", data->config_crc);
      goto mxt_update_cfg_cleanup;

    } else {

      grid_platform_printf("config CRC device %06X != file %06X\n", data->config_crc, config_crc);
    }

  } else {

    grid_platform_printf("info CRC device %06X != file %06X\n", data->info_crc, info_crc);
  }

  // The start address of objects on the device is right after the info block
  uint16_t obj_table_size = sizeof(struct mxt_object) * data->info.object_num;
  uint16_t info_size = MXT_OBJECT_START + obj_table_size + MXT_INFO_CHECKSUM_SIZE;
  cfg.start_ofs = info_size;

  if (data->mem_size <= cfg.start_ofs) {
    grid_platform_printf("mem_size too small, %hu < %hu\n", data->mem_size, cfg.start_ofs);
    goto mxt_update_cfg_error;
  }

  // Allocate memory to store configuration
  cfg.mem_size = data->mem_size - cfg.start_ofs;
  cfg.mem = malloc(cfg.mem_size);
  if (!cfg.mem) {
    grid_platform_printf("failed to allocate cfg.mem\n");
    goto mxt_update_cfg_error;
  }
  memset(cfg.mem, 0, cfg.mem_size);

  // Parse the index of the first device, expect it to be zero
  if (strncmp((char*)&cfg.raw[cfg.raw_pos], MXT_CFG_DEVICE_0, strlen(MXT_CFG_DEVICE_0))) {
    goto mxt_update_cfg_error;
  }
  cfg.raw_pos += strlen(MXT_CFG_DEVICE_0);

  int error = maxtouch_prepare_cfg_mem(data, &cfg);
  if (error) {
    goto mxt_update_cfg_error;
  }

  // Calculate CRC of the received configs (not the raw config file)
  uint16_t crc_start = 0;
  if (data->T71_address) {
    crc_start = data->T71_address;
  } else if (data->T7_address) {
    crc_start = data->T7_address;
  } else {
    grid_platform_printf("could not find CRC start\n");
  }

  if (crc_start > cfg.start_ofs) {

    uint32_t calc_crc = grid_maxtouch_calculate_crc(cfg.mem, crc_start - cfg.start_ofs, cfg.mem_size);

    if (config_crc > 0 && config_crc != calc_crc) {
      grid_platform_printf("config CRC calculated %06X != file %06X\n", calc_crc, config_crc);
    }
  }

  if (maxtouch_upload_cfg_mem(data, &cfg)) {
    grid_platform_printf("config upload error\n");
    goto mxt_update_cfg_error;
  }

  int update_err = grid_maxtouch_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);
  if (update_err) {
    grid_platform_printf("backup error (%d)\n", update_err);
  }

  if (grid_maxtouch_soft_reset(data)) {
    grid_platform_printf("soft reset error\n");
  }

  // Note that there are more steps that are required for an update to take
  // effect on the fly (with a soft reset). Currently, we rely on a hard reset.
  goto mxt_update_cfg_cleanup;

mxt_update_cfg_error:
  ret = -1;
mxt_update_cfg_cleanup:
  if (cfg.raw) {
    free(cfg.raw);
  }
  if (cfg.mem) {
    free(cfg.mem);
  }
  return ret;
}

int grid_maxtouch_update(struct mxt_data* data) {

  struct firmware fw = (struct firmware){
      .size = __common_src_mxt_mxt144u_cfg_raw_len,
      .data = __common_src_mxt_mxt144u_cfg_raw,
  };

  return maxtouch_update_cfg(data, &fw);
}
