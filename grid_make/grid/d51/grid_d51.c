/*
 * grid_d51.c
 *
 * Created: 6/3/2020 5:02:14 PM
 *  Author: WPC-User
 */

#include "grid_d51.h"

#include <assert.h>

void grid_d51_bitmap_write_bit(uint8_t* buffer, uint8_t offset, uint8_t value, uint8_t* changed) {

  uint8_t index = offset / 8;
  uint8_t bit = offset % 8;

  if (value) { // SET BIT

    if ((buffer[index] & (1 << bit)) == 0) {

      buffer[index] |= (1 << bit);
      *changed = 1;
    } else {

      // no change needed
    }
  } else { // CLEAR BIT

    if ((buffer[index] & (1 << bit)) == (1 << bit)) {

      buffer[index] &= ~(1 << bit);
      *changed = 1;
    } else {

      // no change needed
    }
  }
}

void grid_d51_init() {

  rand_sync_enable(&RAND_0);

  grid_d51_dwt_enable(); // debug watch for counting cpu cycles

#ifdef UNITTEST

#else
#endif

  // #define HARDWARETEST

#ifdef HARDWARETEST

#include "grid/grid_hardwaretest.h"

  // grid_d51_nvm_init(&grid_d51_nvm_state, &FLASH_0);

  grid_interface_hardwaretest_main();

#else

#endif
}

void grid_d51_verify_user_row() {

  uint8_t user_area_buffer[512];
  uint8_t user_area_changed_flag = 0;

  _user_area_read(GRID_D51_USER_ROW_BASE, 0, user_area_buffer, 512);

  printf("Reading User Row\r\n");
  _user_area_read(GRID_D51_USER_ROW_BASE, 0, user_area_buffer, 512);

  // BOD33 characteristics datasheet page 1796

  printf("Verifying User Row\r\n");

  // BOD33 Disable Bit => Set 0
  grid_d51_bitmap_write_bit(user_area_buffer, 0, 0, &user_area_changed_flag);

  // BOD33 Level => Set 225 = b11100001
  grid_d51_bitmap_write_bit(user_area_buffer, 1, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 2, 0, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 3, 0, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 4, 0, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 5, 0, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 6, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 7, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 8, 1, &user_area_changed_flag);

  // BOD33 Action => Reset = b01
  grid_d51_bitmap_write_bit(user_area_buffer, 9, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 10, 0, &user_area_changed_flag);

  // BOD33 Hysteresis => Set 15 = b1111
  grid_d51_bitmap_write_bit(user_area_buffer, 11, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 12, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 13, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 14, 1, &user_area_changed_flag);

  // BOOTPROTECT 16kB
  grid_d51_bitmap_write_bit(user_area_buffer, 26, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 27, 0, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 28, 1, &user_area_changed_flag);
  grid_d51_bitmap_write_bit(user_area_buffer, 29, 1, &user_area_changed_flag);

  if (user_area_changed_flag == 1) {

    printf("Updating User Row\r\n");
    _user_area_write(GRID_D51_USER_ROW_BASE, 0, user_area_buffer, 512);

    printf("System Reset\r\n");
    NVIC_SystemReset();
  } else {

    printf("Unchanged User Row\r\n");
  }
}

uint8_t grid_d51_boundary_scan(uint32_t* result_bitmap) {

  result_bitmap[0] = 0;
  result_bitmap[1] = 0;
  result_bitmap[2] = 0;
  result_bitmap[3] = 0;

  // set bit high if there was an error
  uint32_t error_bitmap[4] = {0};
  uint32_t error_count = 0;

  uint8_t PIN_GND = 254;
  uint8_t PIN_VDD = 253;

  uint8_t PIN_RST = 252;
  uint8_t PIN_USB = 251;
  uint8_t PIN_SWD = 250;
  uint8_t PIN_DBG = 249;

  uint8_t PIN_CORE = 248;
  uint8_t PIN_VSW = 247;

  uint8_t SPECIAL_CODES = 240;

  // START OF PACKAGE PIN DEFINITIONS

  uint8_t pins[100] = {255};

  pins[0] = GPIO(GPIO_PORTB, 3);

  pins[1] = GPIO(GPIO_PORTA, 0);
  pins[2] = GPIO(GPIO_PORTA, 1);
  pins[3] = GPIO(GPIO_PORTC, 0);
  pins[4] = GPIO(GPIO_PORTC, 1);
  pins[5] = GPIO(GPIO_PORTC, 2);
  pins[6] = GPIO(GPIO_PORTC, 3);
  pins[7] = GPIO(GPIO_PORTA, 2);
  pins[8] = GPIO(GPIO_PORTA, 3);
  pins[9] = GPIO(GPIO_PORTB, 4);
  pins[10] = GPIO(GPIO_PORTB, 5);
  pins[11] = PIN_GND; // GNDANA
  pins[12] = PIN_VDD; // VDDANA
  pins[13] = GPIO(GPIO_PORTB, 6);
  pins[14] = GPIO(GPIO_PORTB, 7);
  pins[15] = GPIO(GPIO_PORTB, 8);
  pins[16] = GPIO(GPIO_PORTB, 9);
  pins[17] = GPIO(GPIO_PORTA, 4);
  pins[18] = GPIO(GPIO_PORTA, 5);
  pins[19] = GPIO(GPIO_PORTA, 6);
  pins[20] = GPIO(GPIO_PORTA, 7);
  pins[21] = GPIO(GPIO_PORTC, 5);
  pins[22] = GPIO(GPIO_PORTC, 6);
  pins[23] = GPIO(GPIO_PORTC, 7);
  pins[24] = PIN_GND; // GND
  pins[25] = PIN_VDD; // VDDIOB

  pins[26] = GPIO(GPIO_PORTA, 8);
  pins[27] = GPIO(GPIO_PORTA, 9);
  pins[28] = GPIO(GPIO_PORTA, 10);
  pins[29] = GPIO(GPIO_PORTA, 11);
  pins[30] = PIN_VDD; // VDDIOB
  pins[31] = PIN_GND; // GND
  pins[32] = GPIO(GPIO_PORTB, 10);
  pins[33] = GPIO(GPIO_PORTB, 11);
  pins[34] = GPIO(GPIO_PORTB, 12);
  pins[35] = GPIO(GPIO_PORTB, 13);
  pins[36] = GPIO(GPIO_PORTB, 14);
  pins[37] = GPIO(GPIO_PORTB, 15);
  pins[38] = PIN_GND; // GND
  pins[39] = PIN_VDD; // VDDIO
  pins[40] = GPIO(GPIO_PORTC, 10);
  pins[41] = GPIO(GPIO_PORTC, 11);
  pins[42] = GPIO(GPIO_PORTC, 12);
  pins[43] = GPIO(GPIO_PORTC, 13);
  pins[44] = GPIO(GPIO_PORTC, 14);
  pins[45] = GPIO(GPIO_PORTC, 15);
  pins[46] = GPIO(GPIO_PORTA, 12);
  pins[47] = GPIO(GPIO_PORTA, 13);
  pins[48] = GPIO(GPIO_PORTA, 14);
  pins[49] = GPIO(GPIO_PORTA, 15);
  pins[50] = PIN_GND; // GND

  pins[51] = PIN_VDD; // VDDIO
  pins[52] = GPIO(GPIO_PORTA, 16);
  pins[53] = GPIO(GPIO_PORTA, 17);
  pins[54] = GPIO(GPIO_PORTA, 18);
  pins[55] = GPIO(GPIO_PORTA, 19);
  pins[56] = GPIO(GPIO_PORTC, 16);
  pins[57] = GPIO(GPIO_PORTC, 17);
  pins[58] = GPIO(GPIO_PORTC, 18);
  pins[59] = GPIO(GPIO_PORTC, 19);
  pins[60] = GPIO(GPIO_PORTC, 20);
  pins[61] = GPIO(GPIO_PORTC, 21);
  pins[62] = PIN_GND; // GND
  pins[63] = PIN_VDD; // VDDIO
  pins[64] = GPIO(GPIO_PORTB, 16);
  pins[65] = GPIO(GPIO_PORTB, 17);
  pins[66] = GPIO(GPIO_PORTB, 18);
  pins[67] = GPIO(GPIO_PORTB, 19);
  pins[68] = GPIO(GPIO_PORTB, 20);
  pins[69] = GPIO(GPIO_PORTB, 21);
  pins[70] = GPIO(GPIO_PORTA, 20);
  pins[71] = GPIO(GPIO_PORTA, 21);
  pins[72] = GPIO(GPIO_PORTA, 22);
  pins[73] = GPIO(GPIO_PORTA, 23);
  pins[74] = GPIO(GPIO_PORTA, 24);
  pins[75] = GPIO(GPIO_PORTA, 25);

  pins[76] = PIN_GND; // GND
  pins[77] = PIN_VDD; // VDDIO
  pins[78] = GPIO(GPIO_PORTB, 22);
  pins[79] = GPIO(GPIO_PORTB, 23);
  pins[80] = GPIO(GPIO_PORTB, 24);
  pins[81] = GPIO(GPIO_PORTB, 25);
  pins[82] = GPIO(GPIO_PORTC, 24);
  pins[83] = GPIO(GPIO_PORTC, 25);
  pins[84] = GPIO(GPIO_PORTC, 26);
  pins[85] = GPIO(GPIO_PORTC, 27);
  pins[86] = GPIO(GPIO_PORTC, 28);
  pins[87] = GPIO(GPIO_PORTA, 27);
  pins[88] = PIN_RST;  // RESETN
  pins[89] = PIN_CORE; // VDDCORE
  pins[90] = PIN_GND;  // GND
  pins[91] = PIN_VSW;  // VSW
  pins[92] = PIN_VDD;  // VDDIO
  pins[93] = GPIO(GPIO_PORTA, 30);
  pins[94] = GPIO(GPIO_PORTA, 31);
  pins[95] = GPIO(GPIO_PORTB, 30);
  pins[96] = GPIO(GPIO_PORTB, 31);
  pins[97] = GPIO(GPIO_PORTB, 0);
  pins[98] = GPIO(GPIO_PORTB, 1);
  pins[99] = GPIO(GPIO_PORTB, 2);

  // END OF PACKAGE PIN DEFINITIONS
  // START OF GRID COMMON PIN DEFINITIONS

  // HWCFG
  // FLASH
  // SWD
  // USB
  // MAPMODE

  // END OF GRID COMMON PIN DEFINITIONS
  // START OF GRID MODULE SPECIFIC PIN DEFINITIONS

  // END OF GRID MODULE SPECIFIC PIN DEFINITIONS

  // TEST ALL OF THE GPIOs
  for (uint8_t i = 0; i < 100; i++) {

    uint8_t pin_failed = false;

    uint8_t pin_this = pins[i];
    uint8_t pin_prev = pins[(i - 1 + 100) % 100];
    uint8_t pin_next = pins[(i + 1) % 100];

    // pin is real gpio pin
    if (pin_this < SPECIAL_CODES) {

      if (!pin_failed) {
        // SET TO INPUT, PULLDOWN
        gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(pin_this, GPIO_PULL_DOWN);

        // TEST IF pins[i] reads low
        if (gpio_get_pin_level(pin_this) != false) {
          pin_failed = true;
        }

        gpio_set_pin_direction(pin_this, GPIO_DIRECTION_OFF);
        gpio_set_pin_pull_mode(pin_this, GPIO_PULL_OFF);
      }

      if (!pin_failed) {
        // SET TO INPUT, PULLUP
        gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
        gpio_set_pin_pull_mode(pin_this, GPIO_PULL_UP);

        // TEST IF pins[i] reads high
        if (gpio_get_pin_level(pin_this) != true) {
          pin_failed = true;
        }

        gpio_set_pin_direction(pin_this, GPIO_DIRECTION_OFF);
        gpio_set_pin_pull_mode(pin_this, GPIO_PULL_OFF);
      }

      // TEST PREV
      if (!pin_failed) {

        if (pin_prev < SPECIAL_CODES) {

          // SET THIS PIN AS INPUT, PULLDOWN
          gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
          gpio_set_pin_pull_mode(pin_this, GPIO_PULL_DOWN);

          // SET PREVIOUS AS OUTPUT, HIGH
          gpio_set_pin_direction(pin_prev, GPIO_DIRECTION_OUT);
          gpio_set_pin_level(pin_prev, true);
          // READ THISPIN
          if (gpio_get_pin_level(pin_this) != false) {
            pin_failed = true;
          }

          // SET THIS PIN AS INPUT, PULLUP
          gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
          gpio_set_pin_pull_mode(pin_this, GPIO_PULL_UP);
          // SET PREVIOUS AS OUTPUT, LOW
          gpio_set_pin_direction(pin_prev, GPIO_DIRECTION_OUT);
          gpio_set_pin_level(pin_prev, false);
          // READ THISPIN
          if (gpio_get_pin_level(pin_this) != true) {
            pin_failed = true;
          }

          // RESET PREVIOUS TO DEFAULT STATE
          gpio_set_pin_direction(pin_this, GPIO_DIRECTION_OFF);
          gpio_set_pin_direction(pin_prev, GPIO_DIRECTION_OFF);
          gpio_set_pin_pull_mode(pin_this, GPIO_PULL_OFF);
          gpio_set_pin_level(pin_prev, false);
        }
      }

      // TEST NEXT
      if (!pin_failed) {

        if (pin_next < SPECIAL_CODES) {

          // SET THIS PIN AS INPUT, PULLDOWN
          gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
          gpio_set_pin_pull_mode(pin_this, GPIO_PULL_DOWN);

          // SET NEXT AS OUTPUT, HIGH
          gpio_set_pin_direction(pin_next, GPIO_DIRECTION_OUT);
          gpio_set_pin_level(pin_next, true);
          // READ THISPIN
          if (gpio_get_pin_level(pin_this) != false) {
            pin_failed = true;
          }

          // SET THIS PIN AS INPUT, PULLUP
          gpio_set_pin_direction(pin_this, GPIO_DIRECTION_IN);
          gpio_set_pin_pull_mode(pin_this, GPIO_PULL_UP);
          // SET NEXT AS OUTPUT, LOW
          gpio_set_pin_direction(pin_next, GPIO_DIRECTION_OUT);
          gpio_set_pin_level(pin_next, false);
          // READ THISPIN
          if (gpio_get_pin_level(pin_this) != true) {
            pin_failed = true;
          }

          // RESET NEXT TO DEFAULT STATE
          gpio_set_pin_direction(pin_this, GPIO_DIRECTION_OFF);
          gpio_set_pin_direction(pin_next, GPIO_DIRECTION_OFF);
          gpio_set_pin_pull_mode(pin_this, GPIO_PULL_OFF);
          gpio_set_pin_level(pin_next, false);
        }
      }

      // RESET THIS PIN TO DEFAULT STATE

      if (pin_failed) {
        // Sorry, pin failed

        uint8_t error_array_index = (i - 1) / 25;
        uint8_t error_bit_index = (i - 1) % 25;

        error_count++;
        error_bitmap[error_array_index] |= (1 << error_bit_index);

        result_bitmap[0] = error_bitmap[0];
        result_bitmap[1] = error_bitmap[1];
        result_bitmap[2] = error_bitmap[2];
        result_bitmap[3] = error_bitmap[3];
      }
    }
  }
}

void grid_d51_boundary_scan_report(uint32_t* result_bitmap) {

  printf("test.mcu.ATSAMD51N20A\r\n");
  printf("test.hwcfg.%d\r\n", grid_platform_get_hwcfg(&grid_sys_state));

  uint32_t uniqueid[4] = {0};
  grid_platform_get_id(uniqueid);

  printf("test.serialno.%08x %08x %08x %08x\r\n", uniqueid[0], uniqueid[1], uniqueid[2], uniqueid[3]);

  for (uint8_t i = 0; i < 4; i++) {

    delay_ms(10);
    printf("test.boundary.%d.", i);

    for (uint8_t j = 0; j < 32; j++) {

      if (result_bitmap[i] & (1 << j)) {
        printf("1");
      } else {
        printf("0");
      }
    }

    printf("\r\n");
  }
}

uint32_t grid_d51_dwt_enable() {

  if (GRID_D51_DWT_CTRL != 0) { // See if DWT is available
    GRID_D51_DEMCR |= 1 << 24;  // Set bit 24
    GRID_D51_DWT_CYCCNT = 0;
    GRID_D51_DWT_CTRL |= 1 << 0; // Set bit 0
  } else {
    printf("Debug Watch and Trace not supported!\r\n");
  }
}

void grid_d51_nvic_debug_priorities(void) {

  // The default priority is 0 for every interrupt

  printf("Pri MASK    %d\r\n", grid_d51_nvic_get_interrupt_priority_mask());

  printf("RTC         %d\r\n", grid_d51_nvic_get_interrupt_priority(RTC_IRQn));

  printf("USB_0       %d\r\n", grid_d51_nvic_get_interrupt_priority(USB_0_IRQn));
  printf("USB_1       %d\r\n", grid_d51_nvic_get_interrupt_priority(USB_1_IRQn));
  printf("USB_2       %d\r\n", grid_d51_nvic_get_interrupt_priority(USB_2_IRQn));
  printf("USB_3       %d\r\n", grid_d51_nvic_get_interrupt_priority(USB_3_IRQn));

  printf("ADC0_0      %d\r\n", grid_d51_nvic_get_interrupt_priority(ADC0_0_IRQn));
  printf("ADC0_1      %d\r\n", grid_d51_nvic_get_interrupt_priority(ADC0_1_IRQn));

  printf("ADC1_0      %d\r\n", grid_d51_nvic_get_interrupt_priority(ADC1_0_IRQn));
  printf("ADC1_1      %d\r\n", grid_d51_nvic_get_interrupt_priority(ADC1_1_IRQn));

  printf("DMAC_0      %d\r\n", grid_d51_nvic_get_interrupt_priority(DMAC_0_IRQn));
  printf("DMAC_1      %d\r\n", grid_d51_nvic_get_interrupt_priority(DMAC_1_IRQn));
  printf("DMAC_2      %d\r\n", grid_d51_nvic_get_interrupt_priority(DMAC_2_IRQn));
  printf("DMAC_3      %d\r\n", grid_d51_nvic_get_interrupt_priority(DMAC_3_IRQn));
  printf("DMAC_4..31  %d\r\n", grid_d51_nvic_get_interrupt_priority(DMAC_4_IRQn));

  printf("SERCOM3 2   %d\r\n", grid_d51_nvic_get_interrupt_priority(SERCOM3_2_IRQn)); // SPI_UI transfer complete

  printf("EVSYS_0     %d\r\n", grid_d51_nvic_get_interrupt_priority(EVSYS_0_IRQn));
  printf("EVSYS_1     %d\r\n", grid_d51_nvic_get_interrupt_priority(EVSYS_1_IRQn));
  printf("EVSYS_2     %d\r\n", grid_d51_nvic_get_interrupt_priority(EVSYS_2_IRQn));
  printf("EVSYS_3     %d\r\n", grid_d51_nvic_get_interrupt_priority(EVSYS_3_IRQn));
  printf("EVSYS_4..11 %d\r\n", grid_d51_nvic_get_interrupt_priority(EVSYS_4_IRQn));
}

void grid_d51_nvic_set_interrupt_priority(IRQn_Type irqn, uint32_t priority) {

  assert(irqn < 136 + 1);                     // From D51 Datasheet
  assert(priority < (1 << __NVIC_PRIO_BITS)); // From D51 Datasheet

  NVIC_SetPriority(irqn, priority);
}

uint32_t grid_d51_nvic_get_interrupt_priority(IRQn_Type irqn) {

  assert(irqn < 136 + 1); // From D51 Datasheet

  return NVIC_GetPriority(irqn);
}

void grid_d51_nvic_set_interrupt_priority_mask(uint32_t priority) { __set_BASEPRI(priority << (8 - __NVIC_PRIO_BITS)); }

uint32_t grid_d51_nvic_get_interrupt_priority_mask(void) { return __get_BASEPRI() >> (8 - __NVIC_PRIO_BITS); }

uint32_t grid_d51_dwt_cycles_read() { return GRID_D51_DWT_CYCCNT; }

uint8_t grid_fusb302_read_id(struct io_descriptor* i2c_io) {

  printf("fusb start\r\n");

  uint8_t buffer[2] = {0x01, 0x00};

  if (1) {

    i2c_m_async_set_slaveaddr(&SYS_I2C, 0x22, I2C_M_SEVEN);

    const uint16_t n = 1;

    struct i2c_m_async_desc* i2c = &SYS_I2C;
    struct _i2c_m_msg msg;
    int32_t ret;

    msg.addr = i2c->slave_addr;
    msg.len = n;
    msg.flags = 0;
    msg.buffer = buffer;

    /* start transfer then return */
    ret = i2c_m_async_transfer(&i2c->device, &msg);

    if (ret != 0) {
      printf("I2C error\r\n");
    }

    uint32_t cycles = grid_d51_dwt_cycles_read();

    while (i2c->device.service.msg.flags & I2C_M_BUSY) {
      ;
    }

    msg.flags = I2C_M_RD | I2C_M_STOP;
    ret = i2c_m_async_transfer(&i2c->device, &msg);

    if (ret != 0) {
      printf("I2C error\r\n");
    }

    while (i2c->device.service.msg.flags & I2C_M_BUSY) {
      ;
    }

    printf("I2C: %d \r\n", buffer[0]);
  }
}

uint8_t grid_mxt144u_read_id(struct io_descriptor* i2c_io) {

  printf("mxt start\r\n");

  uint8_t buffer[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  if (1) {

    i2c_m_async_set_slaveaddr(&SYS_I2C, 0x4a, I2C_M_SEVEN);

    struct i2c_m_async_desc* i2c = &SYS_I2C;
    struct _i2c_m_msg msg;
    int32_t ret;

    msg.addr = i2c->slave_addr;
    msg.len = 2;
    msg.flags = 0;
    msg.buffer = buffer;

    /* start transfer then return */
    ret = i2c_m_async_transfer(&i2c->device, &msg);

    if (ret != 0) {
      printf("I2C error\r\n");
    }

    while (i2c->device.service.msg.flags & I2C_M_BUSY) {
      ;
    }

    msg.len = 7;
    msg.flags = I2C_M_RD | I2C_M_STOP;
    ret = i2c_m_async_transfer(&i2c->device, &msg);

    if (ret != 0) {
      printf("I2C error\r\n");
    }

    while (i2c->device.service.msg.flags & I2C_M_BUSY) {
      ;
    }

    printf("I2C: %d %d %d %d %d %d %d %d \r\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
  }
}

void grid_platform_printf(char const* fmt, ...) {

  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void grid_platform_printf_nonprint(const uint8_t* src, size_t size) {

  for (size_t i = 0; i < size; ++i) {

    printf(src[i] < 32 ? "[%02hhx]" : "%c", src[i]);
  }
}

uint32_t grid_platform_get_id(uint32_t* return_array) {

  return_array[0] = *(uint32_t*)(GRID_D51_UNIQUE_ID_ADDRESS_0);
  return_array[1] = *(uint32_t*)(GRID_D51_UNIQUE_ID_ADDRESS_1);
  return_array[2] = *(uint32_t*)(GRID_D51_UNIQUE_ID_ADDRESS_2);
  return_array[3] = *(uint32_t*)(GRID_D51_UNIQUE_ID_ADDRESS_3);

  return 1;
}

uint32_t grid_platform_get_hwcfg() {

  // Read the register for the first time, then later just return the saved
  // value

  gpio_set_pin_direction(HWCFG_SHIFT, GPIO_DIRECTION_OUT);
  gpio_set_pin_direction(HWCFG_CLOCK, GPIO_DIRECTION_OUT);
  gpio_set_pin_direction(HWCFG_DATA, GPIO_DIRECTION_IN);

  // LOAD DATA
  gpio_set_pin_level(HWCFG_SHIFT, 0);
  delay_ms(1);
  gpio_set_pin_level(HWCFG_SHIFT, 1);
  delay_ms(1);
  gpio_set_pin_level(HWCFG_SHIFT, 0);

  uint8_t hwcfg_value = 0;

  for (uint8_t i = 0; i < 8; i++) { // now we need to shift in the remaining 7 values

    // SHIFT DATA
    gpio_set_pin_level(HWCFG_SHIFT,
                       1); // This outputs the first value to HWCFG_DATA
    delay_ms(1);

    if (gpio_get_pin_level(HWCFG_DATA)) {

      hwcfg_value |= (1 << i);
    } else {
    }

    if (i != 7) {

      // Clock rise
      gpio_set_pin_level(HWCFG_CLOCK, 1);

      delay_ms(1);

      gpio_set_pin_level(HWCFG_CLOCK, 0);
    }
  }

  return hwcfg_value;
}

uint8_t grid_platform_get_random_8() { return rand_sync_read8(&RAND_0); }

void grid_platform_delay_ms(uint32_t delay_milliseconds) { delay_ms(delay_milliseconds); }

uint8_t grid_platform_get_reset_cause() { return hri_rstc_read_RCAUSE_reg(RSTC); }

uint32_t grid_platform_get_frame_len(uint8_t dir) {

  assert(dir < GRID_PORT_DIR_COUNT);

  return usart_tx_ready[dir] == 0;
}

void grid_platform_send_frame(void* swsr, uint32_t size, uint8_t dir) {

  assert(swsr);
  assert(size > 0);
  assert(size <= GRID_PARAMETER_SPI_TRANSACTION_length - 1);
  assert(dir < GRID_PORT_DIR_COUNT);
  assert(grid_swsr_readable(swsr, size));
  assert(usart_tx_ready[dir]);

  struct grid_swsr_t* tx = (struct grid_swsr_t*)swsr;

  grid_swsr_read(tx, usart_tx_buf[dir], size);

  struct io_descriptor* io_descr;

  switch (dir) {
  case GRID_PORT_NORTH: {
    usart_async_get_io_descriptor(&USART_NORTH, &io_descr);
  } break;
  case GRID_PORT_EAST: {
    usart_async_get_io_descriptor(&USART_EAST, &io_descr);
  } break;
  case GRID_PORT_SOUTH: {
    usart_async_get_io_descriptor(&USART_SOUTH, &io_descr);
  } break;
  case GRID_PORT_WEST: {
    usart_async_get_io_descriptor(&USART_WEST, &io_descr);
  } break;
  default:
    assert(0);
  }

  io_write(io_descr, usart_tx_buf[dir], size);
}

uint8_t grid_platform_disable_grid_transmitter(uint8_t direction) {

  if (direction == GRID_CONST_NORTH) {
    usart_async_disable(&USART_NORTH);
  } else if (direction == GRID_CONST_EAST) {
    usart_async_disable(&USART_EAST);
  } else if (direction == GRID_CONST_SOUTH) {
    usart_async_disable(&USART_SOUTH);
  } else if (direction == GRID_CONST_WEST) {
    usart_async_disable(&USART_WEST);
  } else {
  }
}

uint8_t grid_platform_reset_grid_transmitter(uint8_t direction) {

  if (direction == GRID_CONST_NORTH) {
    grid_d51_uart_port_reset_dma(DMA_NORTH_RX_CHANNEL);
  } else if (direction == GRID_CONST_EAST) {
    grid_d51_uart_port_reset_dma(DMA_EAST_RX_CHANNEL);
  } else if (direction == GRID_CONST_SOUTH) {
    grid_d51_uart_port_reset_dma(DMA_SOUTH_RX_CHANNEL);
  } else if (direction == GRID_CONST_WEST) {
    grid_d51_uart_port_reset_dma(DMA_WEST_RX_CHANNEL);
  } else {
  }
}

uint8_t grid_platform_enable_grid_transmitter(uint8_t direction) {

  if (direction == GRID_CONST_NORTH) {
    usart_async_enable(&USART_NORTH);
  } else if (direction == GRID_CONST_EAST) {
    usart_async_enable(&USART_EAST);
  } else if (direction == GRID_CONST_SOUTH) {
    usart_async_enable(&USART_SOUTH);
  } else if (direction == GRID_CONST_WEST) {
    usart_async_enable(&USART_WEST);
  } else {
  }
}

void grid_platform_system_reset() { NVIC_SystemReset(); }

uint8_t grid_platform_get_adc_bit_depth() { return 16; }

static uint64_t micros = 0;

void grid_platform_rtc_set_micros(uint64_t mic) { micros = mic; }

uint64_t grid_platform_rtc_get_micros(void) { return micros; }

uint64_t grid_platform_rtc_get_diff(uint64_t t1, uint64_t t2) { return t1 - t2; }

uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told) { return grid_platform_rtc_get_micros() - told; }

uint32_t grid_platform_get_cycles() { return grid_d51_dwt_cycles_read(); }

uint32_t grid_platform_get_cycles_per_us() { return 120; }

void* grid_platform_allocate_volatile(size_t size) {

  void* handle = malloc(size);
  if (handle == NULL) {

    printf("MALLOC FAILED");

    while (1) {
    }
  }

  return handle;
}
