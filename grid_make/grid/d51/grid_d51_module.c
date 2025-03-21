#include "grid_d51_module.h"

// Define all of the peripheral interrupt callbacks

/* ============================== GRID_MODULE_INIT()
 * ================================ */

struct grid_port grid_port_array[6] = {0};

struct grid_buffer grid_buffer_tx_array[6] = {0};
struct grid_buffer grid_buffer_rx_array[6] = {0};

char grid_buffer_tx_memory_array[6][GRID_BUFFER_SIZE] = {0};
char grid_buffer_rx_memory_array[6][GRID_BUFFER_SIZE] = {0};

struct grid_doublebuffer grid_doublebuffer_tx_array[6] = {0};
struct grid_doublebuffer grid_doublebuffer_rx_array[6] = {0};

char grid_doublebuffer_tx_memory_array[6][GRID_DOUBLE_BUFFER_TX_SIZE] = {0};
char grid_doublebuffer_rx_memory_array[6][GRID_DOUBLE_BUFFER_RX_SIZE] = {0};

void grid_module_common_init(void) {

  for (uint8_t i = 0; i < 6; i++) {
    uint8_t types[6] = {GRID_PORT_TYPE_USART, GRID_PORT_TYPE_USART, GRID_PORT_TYPE_USART, GRID_PORT_TYPE_USART, GRID_PORT_TYPE_UI, GRID_PORT_TYPE_USB};
    uint8_t directions[6] = {GRID_CONST_NORTH, GRID_CONST_EAST, GRID_CONST_SOUTH, GRID_CONST_WEST, 0, 0};
    grid_port_init(&grid_port_array[i], types[i], directions[i]);

    grid_buffer_tx_array[i].buffer_length = sizeof(grid_buffer_tx_memory_array[i]);
    grid_buffer_tx_array[i].buffer_storage = grid_buffer_tx_memory_array[i];
    grid_buffer_init(&grid_buffer_tx_array[i], sizeof(grid_buffer_tx_memory_array[i]));

    grid_buffer_rx_array[i].buffer_length = sizeof(grid_buffer_rx_memory_array[i]);
    grid_buffer_rx_array[i].buffer_storage = grid_buffer_rx_memory_array[i];
    grid_buffer_init(&grid_buffer_rx_array[i], sizeof(grid_buffer_rx_memory_array[i]));

    grid_doublebuffer_tx_array[i].buffer_size = sizeof(grid_doublebuffer_tx_memory_array[i]);
    grid_doublebuffer_tx_array[i].buffer_storage = &grid_doublebuffer_tx_memory_array[i];

    grid_doublebuffer_rx_array[i].buffer_size = sizeof(grid_doublebuffer_rx_memory_array[i]);
    grid_doublebuffer_rx_array[i].buffer_storage = &grid_doublebuffer_rx_memory_array[i];
  }

  grid_transport_init(&grid_transport_state);

  grid_transport_register_port(&grid_transport_state, &grid_port_array[0]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[1]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[2]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[3]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[4]);
  grid_transport_register_port(&grid_transport_state, &grid_port_array[5]);

  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[0], &grid_buffer_rx_array[0]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[1], &grid_buffer_rx_array[1]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[2], &grid_buffer_rx_array[2]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[3], &grid_buffer_rx_array[3]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[4], &grid_buffer_rx_array[4]);
  grid_transport_register_buffer(&grid_transport_state, &grid_buffer_tx_array[5], &grid_buffer_rx_array[5]);

  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[0], &grid_doublebuffer_rx_array[0]);
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[1], &grid_doublebuffer_rx_array[1]);
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[2], &grid_doublebuffer_rx_array[2]);
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[3], &grid_doublebuffer_rx_array[3]);
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[4], &grid_doublebuffer_rx_array[4]);
  grid_transport_register_doublebuffer(&grid_transport_state, &grid_doublebuffer_tx_array[5], &grid_doublebuffer_rx_array[5]);

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
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevD ||
             grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_RevH) {
    printf("Init Module: EN16\r\n");
    grid_module_en16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevD ||
             grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EN16_ND_RevH) {
    printf("Init Module: EN16 ND\r\n");
    grid_module_en16_init();
  } else if (grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevA || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevD ||
             grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_RevH || grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_ND_RevD ||
             grid_sys_get_hwcfg(&grid_sys_state) == GRID_MODULE_EF44_ND_RevH) {
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
