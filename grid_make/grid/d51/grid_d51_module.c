#include "grid_d51_module.h"

#include "grid_d51_nvm.h"

// Define all of the peripheral interrupt callbacks

/* ============================== GRID_MODULE_INIT()
 * ================================ */

void grid_module_common_init(void) {

  const int PORT_COUNT = 6;
  grid_transport_malloc(&grid_transport_state, PORT_COUNT);

  grid_port_init(&grid_transport_state.ports[0], GRID_PORT_USART, GRID_PORT_NORTH);
  grid_port_init(&grid_transport_state.ports[1], GRID_PORT_USART, GRID_PORT_EAST);
  grid_port_init(&grid_transport_state.ports[2], GRID_PORT_USART, GRID_PORT_SOUTH);
  grid_port_init(&grid_transport_state.ports[3], GRID_PORT_USART, GRID_PORT_WEST);
  grid_port_init(&grid_transport_state.ports[4], GRID_PORT_UI, 0);
  grid_port_init(&grid_transport_state.ports[5], GRID_PORT_USB, 0);

  printf("Common init done\r\n");

  // enable pwr!
  gpio_set_pin_level(UI_PWR_EN, true);

  // set priorities for all of the UI related interrupts

  grid_d51_nvic_set_interrupt_priority(ADC0_0_IRQn, 1);
  grid_d51_nvic_set_interrupt_priority(ADC0_1_IRQn, 1);
  grid_d51_nvic_set_interrupt_priority(ADC1_0_IRQn, 1);
  grid_d51_nvic_set_interrupt_priority(ADC1_1_IRQn, 1);

  grid_d51_nvic_set_interrupt_priority(SERCOM3_0_IRQn, 1);
  grid_d51_nvic_set_interrupt_priority(SERCOM3_1_IRQn, 1);
  grid_d51_nvic_set_interrupt_priority(SERCOM3_2_IRQn, 1); // SERCOM3_2_IRQn handles reading encoders
  grid_d51_nvic_set_interrupt_priority(SERCOM3_3_IRQn, 1);

  // disable ui interrupts
  grid_d51_nvic_set_interrupt_priority_mask(1);

  grid_d51_nvm_mount(&grid_d51_nvm_state, false);

  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevC) {
    printf("Init Module: PO16\r\n");
    grid_module_po16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevC) {
    printf("Init Module: BU16\r\n");
    grid_module_bu16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevA) {
    printf("Init Module: PBF4\r\n");
    grid_module_pbf4_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevA) {
    printf("Init Module: EN16\r\n");
    grid_module_en16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevA) {
    printf("Init Module: EN16 ND\r\n");
    grid_module_en16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevA) {
    printf("Init Module: EF44\r\n");
    grid_module_ef44_init();
  } else {
    printf("Init Module: Unknown Module\r\n");
  }

  printf("Model init done\r\n");

  grid_d51_uart_init(); // uart init after port and transport

  grid_sys_set_bank(&grid_sys_state, 0);
}
