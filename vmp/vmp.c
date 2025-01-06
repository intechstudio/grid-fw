#include "vmp.h"

uint32_t htonl(uint32_t hostlong) { return hostlong; }
uint16_t htons(uint16_t hostshort) { return hostshort; }
uint32_t ntohl(uint32_t netlong) { return netlong; }
uint16_t ntohs(uint16_t netshort) { return netshort; }

enum vmp_err_t vmp_buf_malloc(struct vmp_buf_t* buf, int capacity, int typesize) {
  assert(capacity > 0);
  assert(typesize > 0);

  buf->capacity = capacity;
  buf->typesize = typesize;

  void* evts = vmp_alloc(capacity * typesize);
  if (!evts) {
    return VMP_ERR_MALLOC;
  }

  buf->evts = evts;

  vmp_buf_init(buf);

  return VMP_ERR_NONE;
}

enum vmp_err_t vmp_buf_free(struct vmp_buf_t* buf) {
  if (!buf->evts) {
    return VMP_ERR_FREE;
  }

  vmp_dealloc(buf->evts);
  buf->evts = NULL;

  return VMP_ERR_NONE;
}

void vmp_buf_init(struct vmp_buf_t* buf) {
  buf->size = 0;
  buf->end = 0;
}

size_t vmp_buf_serialize_and_write(struct vmp_buf_t* buf, struct vmp_reg_t* reg) {
  if (!reg->evt_serialized_size) {
    return 0;
  }
  if (!reg->evt_serialize) {
    return 0;
  }
  if (!reg->fwrite) {
    return 0;
  }

  size_t size = 0;

  uint32_t u32;

  u32 = htonl(buf->size);
  size += reg->fwrite(&u32, sizeof(uint32_t));
  u32 = htonl(buf->typesize);
  size += reg->fwrite(&u32, sizeof(uint32_t));

  size_t evt_size = reg->evt_serialized_size();
  uint8_t* evt_buf = vmp_alloc(evt_size);
  if (!evt_buf) {
    return 0;
  }

  for (int i = 0; i < buf->size; ++i) {
    size_t s = reg->evt_serialize(vmp_buf_get(buf, i), evt_buf);
    size += reg->fwrite(evt_buf, s);
  }

  vmp_dealloc(evt_buf);

  return size;
}

size_t vmp_buf_read_and_deserialize(struct vmp_buf_t* buf, struct vmp_reg_t* reg) {
  if (!reg->evt_serialized_size) {
    return 0;
  }
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
  size += reg->fread(&u32, sizeof(uint32_t));
  int typesize = ntohl(u32);

  enum vmp_err_t err = vmp_buf_malloc(buf, capacity, typesize);
  if (err != VMP_ERR_NONE) {
    return 0;
  }

  size_t evt_size = reg->evt_serialized_size();
  uint8_t* evt_buf = vmp_alloc(evt_size);
  if (!evt_buf) {
    vmp_buf_free(buf);
    return 0;
  }

  for (int i = 0; i < capacity; ++i) {
    size += reg->fread(evt_buf, evt_size);
    reg->evt_deserialize(evt_buf, vmp_buf_at(buf, i));
    vmp_buf_incr(buf);
  }

  vmp_dealloc(evt_buf);

  return size;
}
