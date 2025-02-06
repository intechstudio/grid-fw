#ifndef VMP_TAG_H
#define VMP_TAG_H

#include "vmp.h"

enum {
  MAIN = 0,
  TOP,
  BOT,
  VMP_UID_COUNT,
};

char* VMP_ASSOC[] = {
    VMP_NAME_EQ_STR_ENTRY(MAIN),
    VMP_NAME_EQ_STR_ENTRY(TOP),
    VMP_NAME_EQ_STR_ENTRY(BOT),
};

#endif /* VMP_TAG_H */
