/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file or
 * main.c to avoid losing it when reconfiguring.
 */

#include "stdio_start.h"
#include "atmel_start.h"

void STDIO_REDIRECT_0_example(void) {
  /* Print welcome message */
  printf("\r\nHello ATMEL World!\r\n");
}

void stdio_redirect_init(void) {

  usart_sync_enable(&GRID_AUX);
  stdio_io_init(&GRID_AUX.io);
}
