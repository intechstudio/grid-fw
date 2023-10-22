// fuzz_target.cc
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {



  // Pass the input.

  if (Data[10] == 0){
    printf("Data error %s\r\n", Data);
    return -1;
  }

  // Pass the input.
  return 0;  // Values other than 0 and -1 are reserved for future use.
}