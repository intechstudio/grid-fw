# Do not use the built-in inference rules
.SUFFIXES:

CC = arm-none-eabi-gcc
GRID_DEFINES = -DGRID

BUILD_DIR = build
BUILD_DIRS = \
	$(BUILD_DIR)/common/src/c \
	$(BUILD_DIR)/common/dep/littlefs \
	$(BUILD_DIR)/common/dep/lua-5.4.3/src \
	$(BUILD_DIR)/common/dep/tomlc99 \
	$(BUILD_DIR)/common/dep/vmp \
	\
	$(BUILD_DIR)/grid/d51 \
	$(BUILD_DIR)/grid/vmp \
	$(BUILD_DIR)/grid/usb/class/midi \
	$(BUILD_DIR)/grid/usb/class/midi/device \
	\
	$(BUILD_DIR)/asf/hpl/adc \
	$(BUILD_DIR)/asf/usb/class/cdc/device \
	$(BUILD_DIR)/asf/hpl/ramecc \
	$(BUILD_DIR)/asf/samd51a/gcc/gcc \
	$(BUILD_DIR)/asf/hpl/evsys \
	$(BUILD_DIR)/asf/hpl/qspi \
	$(BUILD_DIR)/asf/examples \
	$(BUILD_DIR)/asf/hpl/systick \
	$(BUILD_DIR)/asf/hpl/oscctrl \
	$(BUILD_DIR)/asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source \
	$(BUILD_DIR)/asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0 \
	$(BUILD_DIR)/asf/stdio_redirect \
	$(BUILD_DIR)/asf/hpl/tc \
	$(BUILD_DIR)/asf/hpl/osc32kctrl \
	$(BUILD_DIR)/asf/usb \
	$(BUILD_DIR)/asf/hpl/dmac \
	$(BUILD_DIR)/asf/hal/src \
	$(BUILD_DIR)/asf/stdio_redirect/gcc \
	$(BUILD_DIR)/asf/samd51a/gcc \
	$(BUILD_DIR)/asf/hal/utils/src \
	$(BUILD_DIR)/asf/hpl/wdt \
	$(BUILD_DIR)/asf/hpl/rtc \
	$(BUILD_DIR)/asf/hpl/trng \
	$(BUILD_DIR)/asf/usb/class/hid/device \
	$(BUILD_DIR)/asf/hpl/usb \
	$(BUILD_DIR)/asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/GCC/ARM_CM4F \
	$(BUILD_DIR)/asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/MemMang \
	$(BUILD_DIR)/asf/hpl/mclk \
	$(BUILD_DIR)/asf/hpl/eic \
	$(BUILD_DIR)/asf/hpl/cmcc \
	$(BUILD_DIR)/asf/hpl/gclk \
	$(BUILD_DIR)/asf/usb/device \
	$(BUILD_DIR)/asf/hpl/pm \
	$(BUILD_DIR)/asf/usb/class/msc/device \
	$(BUILD_DIR)/asf/hpl/pac \
	$(BUILD_DIR)/asf/hpl/sercom \
	$(BUILD_DIR)/asf/hpl/core \
	$(BUILD_DIR)/asf/hpl/nvmctrl \
	$(BUILD_DIR)/asf/hpl/dsu

$(BUILD_DIRS): ; mkdir -p $@

SRC_DIR = .
SRCS = \
	common/src/c/grid_ain.c \
	common/src/c/grid_allocator.c \
	common/src/c/grid_asc.c \
	common/src/c/grid_cal.c \
	common/src/c/grid_config.c \
	common/src/c/grid_decode.c \
	common/src/c/grid_led.c \
	common/src/c/grid_littlefs.c \
	common/src/c/grid_lua.c \
	common/src/c/grid_lua_api.c \
	common/src/c/grid_math.c \
	common/src/c/grid_module.c \
	common/src/c/grid_msg.c \
	common/src/c/grid_noflash.c \
	common/src/c/grid_platform.c \
	common/src/c/grid_port.c \
	common/src/c/grid_protocol.c \
	common/src/c/grid_rollid.c \
	common/src/c/grid_swsr.c \
	common/src/c/grid_sys.c \
	common/src/c/grid_transport.c \
	common/src/c/grid_ui.c \
	common/src/c/grid_ui_button.c \
	common/src/c/grid_ui_encoder.c \
	common/src/c/grid_ui_endless.c \
	common/src/c/grid_ui_lcd.c \
	common/src/c/grid_ui_potmeter.c \
	common/src/c/grid_ui_system.c \
	common/src/c/grid_usb.c \
	common/src/c/grid_utask.c \
	common/dep/lua-5.4.3/src/dirent.c \
	common/dep/lua-5.4.3/src/lapi.c \
	common/dep/lua-5.4.3/src/lauxlib.c \
	common/dep/lua-5.4.3/src/lbaselib.c \
	common/dep/lua-5.4.3/src/lcode.c \
	common/dep/lua-5.4.3/src/lcorolib.c \
	common/dep/lua-5.4.3/src/lctype.c \
	common/dep/lua-5.4.3/src/ldblib.c \
	common/dep/lua-5.4.3/src/ldebug.c \
	common/dep/lua-5.4.3/src/ldo.c \
	common/dep/lua-5.4.3/src/ldump.c \
	common/dep/lua-5.4.3/src/lfunc.c \
	common/dep/lua-5.4.3/src/lgc.c \
	common/dep/lua-5.4.3/src/linit.c \
	common/dep/lua-5.4.3/src/liolib.c \
	common/dep/lua-5.4.3/src/llex.c \
	common/dep/lua-5.4.3/src/lmathlib.c \
	common/dep/lua-5.4.3/src/lmem.c \
	common/dep/lua-5.4.3/src/loadlib.c \
	common/dep/lua-5.4.3/src/lobject.c \
	common/dep/lua-5.4.3/src/lopcodes.c \
	common/dep/lua-5.4.3/src/loslib.c \
	common/dep/lua-5.4.3/src/lparser.c \
	common/dep/lua-5.4.3/src/lstate.c \
	common/dep/lua-5.4.3/src/lstring.c \
	common/dep/lua-5.4.3/src/lstrlib.c \
	common/dep/lua-5.4.3/src/ltable.c \
	common/dep/lua-5.4.3/src/ltablib.c \
	common/dep/lua-5.4.3/src/ltm.c \
	common/dep/lua-5.4.3/src/lundump.c \
	common/dep/lua-5.4.3/src/lutf8lib.c \
	common/dep/lua-5.4.3/src/lvm.c \
	common/dep/lua-5.4.3/src/lzio.c \
	common/dep/littlefs/lfs.c \
	common/dep/littlefs/lfs_util.c \
	common/dep/tomlc99/toml.c \
	common/dep/vmp/vmp.c \
	\
	grid/d51/grid_d51.c \
	grid/d51/grid_d51_adc.c \
	grid/d51/grid_d51_encoder.c \
	grid/d51/grid_d51_led.c \
	grid/d51/grid_d51_littlefs.c \
	grid/d51/grid_d51_littlefs_api.c \
	grid/d51/grid_d51_lua.c \
	grid/d51/grid_d51_module.c \
	grid/d51/grid_d51_nvm.c \
	grid/d51/grid_d51_uart.c \
	grid/d51/grid_d51_usb.c \
	grid/grid_d51_module_bu16.c \
	grid/grid_d51_module_en16.c \
	grid/grid_d51_module_pbf4.c \
	grid/grid_d51_module_po16.c \
	grid/grid_d51_module_ef44.c \
	grid/vmp/vmp_def.c \
	grid/usb/class/midi/usb_protocol_midi.c \
	grid/usb/class/midi/device/audiodf_midi.c \
	\
	asf/hal/src/hal_io.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/list.c \
	asf/hpl/pac/hpl_pac.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/GCC/ARM_CM4F/port.c \
	asf/hal/src/hal_i2c_m_async.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/event_groups.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/MemMang/heap_4.c \
	asf/hpl/wdt/hpl_wdt.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/croutine.c \
	asf/usb/usb_protocol.c \
	asf/stdio_redirect/stdio_io.c \
	asf/hal/src/hal_spi_m_dma.c \
	asf/stdio_redirect/gcc/write.c \
	asf/samd51a/gcc/system_samd51.c \
	asf/usb/class/hid/device/hiddf_generic.c \
	asf/hal/src/hal_init.c \
	asf/hpl/evsys/hpl_evsys.c \
	asf/hpl/core/hpl_core_m4.c \
	asf/hal/src/hal_cache.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/stream_buffer.c \
	asf/usb/class/cdc/device/cdcdf_acm.c \
	asf/hal/utils/src/utils_syscalls.c \
	asf/hpl/qspi/hpl_qspi.c \
	asf/hpl/nvmctrl/hpl_nvmctrl.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/queue.c \
	asf/usb_start.c \
	asf/hal/src/hal_timer.c \
	asf/hpl/usb/hpl_usb.c \
	asf/stdio_redirect/gcc/read.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/timers.c \
	asf/hal/src/hal_delay.c \
	asf/hpl/pm/hpl_pm.c \
	asf/hpl/core/hpl_init.c \
	asf/hpl/ramecc/hpl_ramecc.c \
	asf/hal/src/hal_wdt.c \
	asf/usb/class/hid/device/hiddf_mouse.c \
	asf/hal/utils/src/utils_assert.c \
	asf/hpl/dmac/hpl_dmac.c \
	asf/hal/src/hal_adc_async.c \
	asf/hpl/oscctrl/hpl_oscctrl.c \
	asf/hal/src/hal_rand_sync.c \
	asf/hpl/trng/hpl_trng.c \
	asf/usb/class/hid/device/hiddf_keyboard.c \
	asf/hal/src/hal_usart_sync.c \
	asf/hpl/mclk/hpl_mclk.c \
	asf/hpl/gclk/hpl_gclk.c \
	asf/stdio_start.c \
	asf/hal/src/hal_flash.c \
	asf/hal/src/hal_spi_m_async.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/rtos_port.c \
	asf/hal/src/hal_crc_sync.c \
	asf/hal/src/hal_usb_device.c \
	asf/rtos_start.c \
	asf/hal/utils/src/utils_list.c \
	asf/hpl/osc32kctrl/hpl_osc32kctrl.c \
	asf/hpl/rtc/hpl_rtc.c \
	asf/examples/driver_examples.c \
	asf/driver_init.c \
	asf/hal/src/hal_usart_async.c \
	asf/usb/class/msc/device/mscdf.c \
	asf/hpl/sercom/hpl_sercom.c \
	asf/hal/utils/src/utils_ringbuffer.c \
	asf/hal/src/hal_atomic.c \
	asf/grid_d51n20a.c \
	asf/hal/src/hal_gpio.c \
	asf/hal/src/hal_ext_irq.c \
	asf/hpl/eic/hpl_eic.c \
	asf/hal/src/hal_qspi_dma.c \
	asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/tasks.c \
	asf/hal/src/hal_evsys.c \
	asf/hal/utils/src/utils_event.c \
	asf/hal/src/hal_sleep.c \
	asf/hpl/adc/hpl_adc.c \
	asf/samd51a/gcc/gcc/startup_samd51.c \
	asf/atmel_start.c \
	asf/usb/device/usbdc.c \
	asf/hpl/tc/hpl_tc.c \
	asf/hpl/cmcc/hpl_cmcc.c \
	asf/hpl/systick/hpl_systick.c \
	asf/hpl/dsu/hpl_dsu.c

INCLUDES = \
	-I"common/src/c" \
	-I"common/dep" \
	-I"common/dep/littlefs" \
	-I"common/dep/lua-5.4.3" \
	-I"common/dep/proto" \
	-I"common/dep/tomlc99" \
	-I"common/dep/vmp" \
	-I"common/build/lua" \
	\
	-I"grid" \
	-I"grid/d51" \
	-I"grid/vmp" \
	-I"grid/usb/class/midi" \
	-I"grid/usb/class/midi/device" \
	\
	-I"asf/config" \
	-I"asf/examples" \
	-I"asf/hal/include" \
	-I"asf/hal/utils/include" \
	-I"asf/hpl/adc" \
	-I"asf/hpl/cmcc" \
	-I"asf/hpl/core" \
	-I"asf/hpl/dmac" \
	-I"asf/hpl/eic" \
	-I"asf/hpl/dsu" \
	-I"asf/hpl/evsys" \
	-I"asf/hpl/gclk" \
	-I"asf/hpl/mclk" \
	-I"asf/hpl/nvmctrl" \
	-I"asf/hpl/osc32kctrl" \
	-I"asf/hpl/oscctrl" \
	-I"asf/hpl/pac" \
	-I"asf/hpl/pm" \
	-I"asf/hpl/port" \
	-I"asf/hpl/qspi" \
	-I"asf/hpl/ramecc" \
	-I"asf/hpl/rtc" \
	-I"asf/hpl/sercom" \
	-I"asf/hpl/systick" \
	-I"asf/hpl/tc" \
	-I"asf/hpl/trng" \
	-I"asf/hpl/usb" \
	-I"asf/hpl/wdt" \
	-I"asf/hri" \
	-I"asf/" \
	-I"asf/config" \
	-I"asf/thirdparty/RTOS" \
	-I"asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0" \
	-I"asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/include" \
	-I"asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/GCC/ARM_CM4F" \
	-I"asf/thirdparty/RTOS/freertos/FreeRTOSV10.0.0/module_config" \
	-I"asf/" \
	-I"asf/config" \
	-I"asf/usb" \
	-I"asf/usb/class/cdc" \
	-I"asf/usb/class/cdc/device" \
	-I"asf/usb/class/composite/device" \
	-I"asf/usb/class/hid" \
	-I"asf/usb/class/hid/device" \
	-I"asf/usb/class/hub" \
	-I"asf/usb/class/msc" \
	-I"asf/usb/class/msc/device" \
	-I"asf/usb/class/vendor" \
	-I"asf/usb/device" \
	-I"asf/" \
	-I"asf/config" \
	-I"asf/stdio_redirect" \
	-I"asf/" \
	-I"asf/CMSIS/Core/Include" \
	-I"asf/samd51a/include"

DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c | $(BUILD_DIRS)
	$(CC) -x c -mthumb \
	$(GRID_DEFINES) \
	-DDEBUG -O2 -ffunction-sections -mlong-calls -g3 -Wall -c -std=gnu99 \
	-D__SAMD51N20A__ -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 \
	$(INCLUDES) \
	-MM -MF "$@" -MT "$@" "$<"

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/%.d | $(BUILD_DIRS)
	$(CC) -x c -mthumb \
	$(GRID_DEFINES) \
	-DDEBUG -O2 -ffunction-sections -mlong-calls -g3 -Wall -c -std=gnu99 \
	-D__SAMD51N20A__ -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 \
	$(INCLUDES) \
	-o "$@" "$<"

OUTPUT_FILE_NAME=grid_d51n20a
OUTPUT_FILE_PATH=$(BUILD_DIR)/$(OUTPUT_FILE_NAME)

$(OUTPUT_FILE_PATH).bin: $(OBJS) $(DEPS) | $(BUILD_DIRS)
	$(CC) -o $(OUTPUT_FILE_PATH).elf $(OBJS) \
	-Wl,--start-group -lm -Wl,--end-group -mthumb \
	-Wl,-Map="$(OUTPUT_FILE_PATH).map" --specs=nosys.specs \
	-Wl,--gc-sections -Wl,--print-memory-usage -mcpu=cortex-m4 \
	-u _printf_float \
	-T"../grid/samd51n20a_flash_bootloader.ld" \
	-L"../samd51a/gcc/gcc" \

	@echo Finished building target: $@

	"arm-none-eabi-objcopy" -O binary "$(OUTPUT_FILE_PATH).elf" "$(OUTPUT_FILE_PATH).bin"
	"arm-none-eabi-objcopy" -O ihex -R .eeprom -R .fuse -R .lock -R .signature  \
        "$(OUTPUT_FILE_PATH).elf" "$(OUTPUT_FILE_PATH).hex"
	"arm-none-eabi-objcopy" -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma \
        .eeprom=0 --no-change-warnings -O binary "$(OUTPUT_FILE_PATH).elf" \
        "$(OUTPUT_FILE_PATH).eep" || exit 0
	"arm-none-eabi-objdump" -h -S "$(OUTPUT_FILE_PATH).elf" > "$(OUTPUT_FILE_PATH).lss"
	"arm-none-eabi-size" "$(OUTPUT_FILE_PATH).elf"

UF2_NAME=grid_d51n20a.uf2
$(BUILD_DIR)/$(UF2_NAME): $(OUTPUT_FILE_PATH).bin
	python3 ../../tools/uf2conv/uf2conv.py -c -b 0x4000 -o "$@" "$<"

.PHONY: clean
clean:
	rm -rf ./build

.PHONY: all
all: $(BUILD_DIR)/$(UF2_NAME)

include $(DEPS)

# debug with autogdb
# arm-none-eabi-gdb -iex "target extended-remote localhost:3333" $(OUTPUT_FILE_NAME).elf
