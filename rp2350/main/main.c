#include <stdio.h>

#include "pico/stdlib.h"

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

int main() {
  stdio_init_all();

  const uint led = PICO_DEFAULT_LED_PIN;
  gpio_init(led);
  gpio_set_dir(led, GPIO_OUT);

  uint32_t count = 0;
  while (true) {
    gpio_put(led, 1);
    sleep_ms(250);
    gpio_put(led, 0);
    sleep_ms(250);

    printf("grid rp2350 heartbeat: %lu\n", (unsigned long)count++);
  }
}
