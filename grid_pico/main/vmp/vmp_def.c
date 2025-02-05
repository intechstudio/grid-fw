#include "vmp_def.h"

struct vmp_buf_t vmp;

#define hex_to_uint8_range(h, min, max, add) (((h) >= (min) && (h) <= (max)) * ((h) - (min) + (add)))

uint8_t hex_to_uint8(char h[2]) {
  uint8_t hi = hex_to_uint8_range(h[0], '0', '9', 0) + hex_to_uint8_range(h[0], 'a', 'f', 10);
  uint8_t lo = hex_to_uint8_range(h[1], '0', '9', 0) + hex_to_uint8_range(h[1], 'a', 'f', 10);

  return (hi << 4) + lo;
}

static char uint4_to_hex[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

void uint8_to_hex(uint8_t u, char h[2]) {
  h[0] = uint4_to_hex[u >> 4];
  h[1] = uint4_to_hex[u & 0xf];
}

size_t vmp_evt_serialize(void* evt, uint8_t* dest) {
  struct vmp_evt_t* event = evt;
  size_t size = 0;

  *((uint16_t*)(dest + size)) = htons((event)->uid);
  size += sizeof(uint16_t);
  *((uint16_t*)(dest + size)) = htons((event)->line);
  size += sizeof(uint16_t);
  *((uint32_t*)(dest + size)) = htonl((event)->time);
  size += sizeof(uint32_t);

  return size;
}

size_t vmp_evt_deserialize(uint8_t* src, void* evt) {
  struct vmp_evt_t* event = evt;
  size_t size = 0;

  event->uid = ntohs(*((uint16_t*)(src + size)));
  size += sizeof(uint16_t);
  event->line = ntohs(*((uint16_t*)(src + size)));
  size += sizeof(uint16_t);
  event->time = ntohl(*((uint32_t*)(src + size)));
  size += sizeof(uint32_t);

  return size;
}

size_t vmp_evt_serialized_size() {
  return sizeof(uint16_t) + // uid
         sizeof(uint16_t) + // line
         sizeof(uint32_t) + // time
         0;
}

size_t vmp_fwrite(void* ptr, size_t size) {
  uint8_t* u8 = ptr;
  char hex[2];

  size_t s = 0;
  for (int i = 0; i < size; ++i) {
    uint8_to_hex(u8[i], hex);
    s += printf("%c%c", hex[0], hex[1]);
  }

  return s;
}

size_t vmp_fread(void* ptr, size_t size) {
  uint8_t* u8 = ptr;
  char hex[2];

  size_t s = 0;
  for (int i = 0; i < size; ++i) {
    s += fread(hex, 1, 2, stdin);
    u8[i] = hex_to_uint8(hex);
  }

  assert(s % 2 == 0);

  return s / 2;
}

enum vmp_err_t vmp_uid_str_malloc(struct vmp_uid_str_t* uid_str, int capacity) {
  assert(capacity > 0);

  uid_str->capacity = capacity;

  char(*strs)[VMP_UID_STR_MAX] = vmp_alloc(capacity * VMP_UID_STR_MAX);
  if (!strs) {
    return VMP_ERR_MALLOC;
  }

  uid_str->strs = strs;

  return VMP_ERR_NONE;
}

enum vmp_err_t vmp_uid_str_free(struct vmp_uid_str_t* uid_str) {
  if (!uid_str->strs) {
    return VMP_ERR_FREE;
  }

  vmp_dealloc(uid_str->strs);
  uid_str->strs = NULL;

  return VMP_ERR_NONE;
}

size_t vmp_write_u8_str(vmp_fwrite_t write, char* s) {
  size_t len = strlen(s);
  if (len > VMP_UID_STR_MAX - 1) {
    return 0;
  }

  size_t size = 0;

  uint8_t u8 = len;
  size += write(&u8, sizeof(uint8_t));

  size += write(s, u8);

  return size;
}

size_t vmp_read_u8_str(vmp_fread_t read, char* s) {
  size_t size = 0;

  uint8_t u8;
  size += read(&u8, sizeof(uint8_t));

  size += read(s, u8);
  s[u8] = '\0';

  return size;
}

size_t vmp_uid_str_serialize_and_write(int uids, char** strs, struct vmp_reg_t* reg) {
  if (!reg->evt_serialize) {
    return 0;
  }
  if (!reg->fwrite) {
    return 0;
  }

  size_t size = 0;

  uint32_t u32;

  u32 = htonl(uids);
  size += reg->fwrite(&u32, sizeof(uint32_t));

  for (int i = 0; i < uids; ++i) {
    size += vmp_write_u8_str(reg->fwrite, strs[i]);
  }

  return size;
}

size_t vmp_uid_str_read_and_deserialize(struct vmp_uid_str_t* uid_str, struct vmp_reg_t* reg) {
  if (!reg->evt_deserialize) {
    return 0;
  }
  if (!reg->fread) {
    return 0;
  }

  size_t size = 0;

  uint32_t u32;

  size += reg->fread(&u32, sizeof(uint32_t));
  int capacity = ntohl(u32);

  enum vmp_err_t err = vmp_uid_str_malloc(uid_str, capacity);
  if (err != VMP_ERR_NONE) {
    return 0;
  }

  for (int i = 0; i < capacity; ++i) {
    size += vmp_read_u8_str(reg->fread, uid_str->strs[i]);
  }

  return size;
}

size_t vmp_serialize_start(struct vmp_reg_t* reg) {
  if (!reg->fwrite) {
    return 0;
  }

  return reg->fwrite(VMP_START, strlen(VMP_START));
}

size_t vmp_serialize_close(struct vmp_reg_t* reg) {
  if (!reg->fwrite) {
    return 0;
  }

  return reg->fwrite(VMP_CLOSE, strlen(VMP_CLOSE));
}

enum vmp_err_t vmp_evt_to_vis(struct vmp_buf_t* src, struct vmp_buf_t* dest) {
  if (src->capacity != dest->capacity) {
    return VMP_ERR_PARAM;
  }
  if (src->typesize != sizeof(struct vmp_evt_t)) {
    return VMP_ERR_PARAM;
  }
  if (dest->typesize != sizeof(struct vmp_vis_t)) {
    return VMP_ERR_PARAM;
  }

  if (dest->size != 0) {
    return VMP_ERR_PARAM;
  }
  if (src->size < 1) {
    return VMP_ERR_PARAM;
  }

  struct vmp_evt_t* zero_evt = VMP_EVT(vmp_buf_get(src, 0));
  *VMP_VIS(vmp_buf_get(dest, 0)) = (struct vmp_vis_t){
      .evt = *zero_evt,
      .time_abs = 0,
      .time_rel = 0,
  };

  for (int i = 1; i < src->capacity; ++i) {
    struct vmp_evt_t* prev_evt = VMP_EVT(vmp_buf_get(src, i - 1));
    struct vmp_evt_t* evt = VMP_EVT(vmp_buf_get(src, i));
    *VMP_VIS(vmp_buf_get(dest, i)) = (struct vmp_vis_t){
        .evt = *evt,
        .time_abs = evt->time - zero_evt->time,
        .time_rel = evt->time - prev_evt->time,
    };
    vmp_buf_incr(dest);
  }

  return VMP_ERR_NONE;
}
