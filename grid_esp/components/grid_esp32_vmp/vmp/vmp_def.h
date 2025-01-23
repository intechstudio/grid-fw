#ifndef VMP_DEF_H
#define VMP_DEF_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

extern void grid_platform_printf(char const* fmt, ...);

#include "vmp.h"

extern struct vmp_buf_t vmp;

#define vmp_push(...) vmp_buf_push(&vmp, __VA_ARGS__)

struct vmp_evt_t {
  uint16_t uid;
  uint16_t line;
  uint32_t time;
  uint16_t scln;
};

#define VMP_EVT(ptr) ((struct vmp_evt_t*)(ptr))

#define VMP_EVT_WRITE(ptr, id)                                                                                                                                                                         \
  {                                                                                                                                                                                                    \
    VMP_EVT(ptr)->uid = (id);                                                                                                                                                                          \
    VMP_EVT(ptr)->line = __LINE__;                                                                                                                                                                     \
    VMP_EVT(ptr)->time = grid_platform_rtc_get_micros();                                                                                                                                                             \
    VMP_EVT(ptr)->scln = 0/*vmp_get_scanline()*/;                                                                                                                                                             \
  }

size_t vmp_evt_serialize(void* evt, uint8_t* dest);
size_t vmp_evt_deserialize(uint8_t* src, void* evt);
size_t vmp_evt_serialized_size();

size_t vmp_fwrite(void* ptr, size_t size);
size_t vmp_fread(void* ptr, size_t size);

enum {
  VMP_UID_STR_MAX = 0x100,
};

struct vmp_uid_str_t {
  int capacity;
  char (*strs)[VMP_UID_STR_MAX];
};

enum vmp_err_t vmp_uid_str_malloc(struct vmp_uid_str_t* uid_str, int capacity);
enum vmp_err_t vmp_uid_str_free(struct vmp_uid_str_t* uid_str);
size_t vmp_uid_str_serialize_and_write(int uids, char** strs, struct vmp_reg_t* reg);
size_t vmp_uid_str_read_and_deserialize(struct vmp_uid_str_t* uid_str, struct vmp_reg_t* reg);

#define VMP_START "\xab\xad\xca\xfe"
#define VMP_CLOSE "\xab\xad\xc0\xde"

size_t vmp_serialize_start(struct vmp_reg_t* reg);
size_t vmp_serialize_close(struct vmp_reg_t* reg);

struct vmp_vis_t {
  struct vmp_evt_t evt;
  uint32_t time_abs;
  uint32_t time_rel;
};

#define VMP_VIS(ptr) ((struct vmp_vis_t*)(ptr))

enum vmp_err_t vmp_evt_to_vis(struct vmp_buf_t* src, struct vmp_buf_t* dest);

#endif /* VMP_DEF_H */
