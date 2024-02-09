#include "grid_d51_module.h"

// Define all of the peripheral interrupt callbacks

/* ============================== GRID_MODULE_INIT()
 * ================================ */

void grid_module_common_init(void) {

  grid_transport_init(&grid_transport_state);

  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_NORTH));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_EAST));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_SOUTH));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USART, GRID_CONST_WEST));

  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_UI, 0));
  grid_transport_register_port(&grid_transport_state, grid_port_allocate_init(GRID_PORT_TYPE_USB, 0));

  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));
  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));
  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));
  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));

  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));
  grid_transport_register_doublebuffer(&grid_transport_state, grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_TX_SIZE), grid_doublebuffer_allocate_init(GRID_DOUBLE_BUFFER_RX_SIZE));

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

  if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PO16_RevC) {
    printf("Init Module: PO16\r\n");
    grid_module_po16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevB || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_BU16_RevC) {
    printf("Init Module: BU16\r\n");
    grid_module_bu16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_PBF4_RevA) {
    printf("Init Module: PBF4\r\n");
    grid_module_pbf4_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD) {
    printf("Init Module: EN16\r\n");
    grid_module_en16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD) {
    printf("Init Module: EN16 ND\r\n");
    grid_module_en16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD) {
    printf("Init Module: EF44\r\n");
    grid_module_ef44_init();
  } else {
    printf("Init Module: Unknown Module\r\n");
  }

  printf("Model init done\r\n");

  grid_d51_uart_init(); // uart init after port and transport

  grid_sys_set_bank(&grid_sys_state, 0);

  grid_d51_nvm_init(&grid_d51_nvm_state, &FLASH_0);
}
