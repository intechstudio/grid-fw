#ifndef VMP_H
#define VMP_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void* (*VMP_ALLOC)(size_t) = malloc;
static void (*VMP_DEALLOC)(void*) = free;

#define vmp_alloc(a) VMP_ALLOC(a)
#define vmp_dealloc(a) VMP_DEALLOC(a)

typedef size_t (*vmp_evt_serialized_size_t)();
typedef size_t (*vmp_serialize_t)(void* self, uint8_t* dest);
typedef size_t (*vmp_deserialize_t)(uint8_t* src, void* self);
typedef size_t (*vmp_fread_t)(void* ptr, size_t size);
typedef size_t (*vmp_fwrite_t)(void* ptr, size_t size);

struct vmp_reg_t {
  vmp_evt_serialized_size_t evt_serialized_size;
  vmp_serialize_t evt_serialize;
  vmp_deserialize_t evt_deserialize;
  vmp_fwrite_t fwrite;
  vmp_fread_t fread;
};

uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

enum vmp_err_t {
  VMP_ERR_NONE = 0,
  VMP_ERR_MALLOC,
  VMP_ERR_FREE,
  VMP_ERR_PARAM,
};

struct vmp_buf_t {
  int capacity;
  int typesize;
  int size;
  int end;
  void* evts;
};

enum vmp_err_t vmp_buf_malloc(struct vmp_buf_t* buf, int capacity, int typesize);
enum vmp_err_t vmp_buf_free(struct vmp_buf_t* buf);
void vmp_buf_init(struct vmp_buf_t* buf);
size_t vmp_buf_serialize_and_write(struct vmp_buf_t* buf, struct vmp_reg_t* reg);
size_t vmp_buf_read_and_deserialize(struct vmp_buf_t* buf, struct vmp_reg_t* reg);

#define vmp_buf_at(buf, i) (((uint8_t*)((buf)->evts)) + (i) * (buf)->typesize)

#define vmp_buf_get(buf, i) (vmp_buf_at(buf, ((buf)->capacity + (buf)->end - (buf)->size + (i)) % (buf)->capacity))

#define vmp_buf_incr(buf)                                                                                                                                                                              \
  {                                                                                                                                                                                                    \
    (buf)->end = ((buf)->end + 1) % (buf)->capacity;                                                                                                                                                   \
    (buf)->size += (buf)->size < (buf)->capacity;                                                                                                                                                      \
  }

#define vmp_buf_push(buf, ...)                                                                                                                                                                         \
  {                                                                                                                                                                                                    \
    VMP_EVT_WRITE(vmp_buf_at(buf, (buf)->end), __VA_ARGS__)                                                                                                                                            \
    vmp_buf_incr(buf)                                                                                                                                                                                  \
  }

#define VMP_NAME_EQ_STR_ENTRY(name) [name] = #name

#endif /* VMP_H */
