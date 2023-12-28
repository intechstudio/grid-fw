// fuzz_target.cc
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {



  // Pass the input.

  if (size<10){
    return 0;
  }
  else{
    return 0;
  }

  if (size > 0 && data[0] == 'H')
    if (size > 1 && data[1] == 'I')
       if (size > 2 && data[2] == '!')
       __builtin_trap();

  // Pass the input.
  return 0;  // Values other than 0 and -1 are reserved for future use.
}
