#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "vmp_def.h"

int main() {
  struct vmp_reg_t reg = {
      .evt_serialized_size = vmp_evt_serialized_size,
      .evt_deserialize = vmp_evt_deserialize,
      .fread = vmp_fread,
  };

  struct vmp_uid_str_t uid_str;
  vmp_buf_read_and_deserialize(&vmp, &reg);
  vmp_uid_str_read_and_deserialize(&uid_str, &reg);

  struct vmp_buf_t vmp_vis;
  vmp_buf_malloc(&vmp_vis, vmp.capacity, sizeof(struct vmp_vis_t));
  vmp_evt_to_vis(&vmp, &vmp_vis);

  for (int i = 0; i < vmp_vis.capacity; ++i) {
    struct vmp_vis_t* vis = VMP_VIS(vmp_buf_get(&(vmp_vis), i));
    struct vmp_evt_t* evt = &vis->evt;
    printf("%02u id, %6u abs, %6u rel, %6u pre, %3u scl, %4u:%s,\n",
      evt->uid, evt->time, vis->time_abs, vis->time_rel, evt->scln, evt->line, uid_str.strs[evt->uid]);
  }

  vmp_buf_free(&vmp_vis);

  vmp_uid_str_free(&uid_str);
  vmp_buf_free(&vmp);

  return 0;
}
