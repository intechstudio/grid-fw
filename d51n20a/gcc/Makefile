CC = arm-none-eabi-gcc

GRID_DEFINES = -DGRID

###################################
# Platform-specific code for GRID #
###################################

D51_INC_DIR = \
-I "../grid" \
-I "../grid/d51" \
-I "../grid/vmp" \
-I "../grid/usb/class/midi" \
-I "../grid/usb/class/midi/device" \

D51_SRC_DIR = ../grid
D51_SRCS = \
d51/grid_d51.c \
d51/grid_d51_adc.c \
d51/grid_d51_encoder.c \
d51/grid_d51_led.c \
d51/grid_d51_littlefs.c \
d51/grid_d51_littlefs_api.c \
d51/grid_d51_lua.c \
d51/grid_d51_module.c \
d51/grid_d51_nvm.c \
d51/grid_d51_uart.c \
d51/grid_d51_usb.c \
grid_d51_module_bu16.c \
grid_d51_module_en16.c \
grid_d51_module_pbf4.c \
grid_d51_module_po16.c \
grid_d51_module_ef44.c \
vmp/vmp_def.c \
usb/class/midi/usb_protocol_midi.c \
usb/class/midi/device/audiodf_midi.c \

D51_OBJ_DIRS = \
build/grid/d51 \
build/grid/vmp \
build/grid/usb/class/midi \
build/grid/usb/class/midi/device \

$(D51_OBJ_DIRS): ; mkdir -p $@

D51_OBJ_DIR = build/grid
$(D51_OBJ_DIR): $(D51_OBJ_DIRS)
D51_OBJS = $(D51_SRCS:%.c=$(D51_OBJ_DIR)/%.o)

$(D51_OBJ_DIR)/%.o: $(D51_SRC_DIR)/%.c | $(D51_OBJ_DIR)
	$(CC) -x c -mthumb \
	$(GRID_DEFINES) \
	-DDEBUG -Os -ffunction-sections -mlong-calls -g3 -Wall -c -std=gnu99 \
	-D__SAMD51N20A__ -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 \
	$(D51_INC_DIR) \
	-MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -o "$@" "$<"

OBJS_AS_ARGS += $(D51_SRCS:%.c=$(D51_OBJ_DIR)/%.o)

COM_INC_DIR = \
-I "../../common/src/c" \
-I "../../common/dep" \
-I "../../common/dep/littlefs" \
-I "../../common/dep/lua-5.4.3" \
-I "../../common/dep/proto" \
-I "../../common/dep/tomlc99" \
-I "../../common/dep/vmp" \
-I "../../common/build/lua" \

D51_INC_DIR += $(COM_INC_DIR)

######################################
# Platform-independent code for GRID #
######################################

COM_SRC_DIR = ../../common
COM_SRCS = \
src/c/grid_ain.c \
src/c/grid_allocator.c \
src/c/grid_asc.c \
src/c/grid_cal.c \
src/c/grid_config.c \
src/c/grid_decode.c \
src/c/grid_led.c \
src/c/grid_littlefs.c \
src/c/grid_lua.c \
src/c/grid_lua_api.c \
src/c/grid_math.c \
src/c/grid_module.c \
src/c/grid_msg.c \
src/c/grid_noflash.c \
src/c/grid_platform.c \
src/c/grid_port.c \
src/c/grid_protocol.c \
src/c/grid_rollid.c \
src/c/grid_swsr.c \
src/c/grid_sys.c \
src/c/grid_transport.c \
src/c/grid_ui.c \
src/c/grid_ui_button.c \
src/c/grid_ui_encoder.c \
src/c/grid_ui_endless.c \
src/c/grid_ui_lcd.c \
src/c/grid_ui_potmeter.c \
src/c/grid_ui_system.c \
src/c/grid_usb.c \
src/c/grid_utask.c \
dep/lua-5.4.3/src/dirent.c \
dep/lua-5.4.3/src/lapi.c \
dep/lua-5.4.3/src/lauxlib.c \
dep/lua-5.4.3/src/lbaselib.c \
dep/lua-5.4.3/src/lcode.c \
dep/lua-5.4.3/src/lcorolib.c \
dep/lua-5.4.3/src/lctype.c \
dep/lua-5.4.3/src/ldblib.c \
dep/lua-5.4.3/src/ldebug.c \
dep/lua-5.4.3/src/ldo.c \
dep/lua-5.4.3/src/ldump.c \
dep/lua-5.4.3/src/lfunc.c \
dep/lua-5.4.3/src/lgc.c \
dep/lua-5.4.3/src/linit.c \
dep/lua-5.4.3/src/liolib.c \
dep/lua-5.4.3/src/llex.c \
dep/lua-5.4.3/src/lmathlib.c \
dep/lua-5.4.3/src/lmem.c \
dep/lua-5.4.3/src/loadlib.c \
dep/lua-5.4.3/src/lobject.c \
dep/lua-5.4.3/src/lopcodes.c \
dep/lua-5.4.3/src/loslib.c \
dep/lua-5.4.3/src/lparser.c \
dep/lua-5.4.3/src/lstate.c \
dep/lua-5.4.3/src/lstring.c \
dep/lua-5.4.3/src/lstrlib.c \
dep/lua-5.4.3/src/ltable.c \
dep/lua-5.4.3/src/ltablib.c \
dep/lua-5.4.3/src/ltm.c \
dep/lua-5.4.3/src/lundump.c \
dep/lua-5.4.3/src/lutf8lib.c \
dep/lua-5.4.3/src/lvm.c \
dep/lua-5.4.3/src/lzio.c \
dep/littlefs/lfs.c \
dep/littlefs/lfs_util.c \
dep/tomlc99/toml.c \
dep/vmp/vmp.c \

COM_OBJ_DIRS = \
build/common/src/c \
build/common/dep/littlefs \
build/common/dep/lua-5.4.3/src \
build/common/dep/tomlc99 \
build/common/dep/vmp \

$(COM_OBJ_DIRS): ; mkdir -p $@

COM_OBJ_DIR = build/common
$(COM_OBJ_DIR): $(COM_OBJ_DIRS)
COM_OBJS = $(COM_SRCS:%.c=$(COM_OBJ_DIR)/%.o)

$(COM_OBJ_DIR)/%.o: $(COM_SRC_DIR)/%.c | $(COM_OBJ_DIR)
	$(CC) -x c -mthumb \
	$(GRID_DEFINES) \
	-DDEBUG -Os -ffunction-sections -mlong-calls -g3 -Wall -c -std=gnu99 \
	-D__SAMD51N20A__ -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 \
	$(COM_INC_DIR) \
	-MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -o "$@" "$<"

OBJS_AS_ARGS += $(COM_SRCS:%.c=$(COM_OBJ_DIR)/%.o)

################################################################################
# Automatically-generated file. Do not edit!
################################################################################

ifdef SystemRoot
	SHELL = cmd.exe
	MK_DIR = mkdir
else
	ifeq ($(shell uname), Linux)
		MK_DIR = mkdir -p
	endif

	ifeq ($(shell uname | cut -d _ -f 1), CYGWIN)
		MK_DIR = mkdir -p
	endif

	ifeq ($(shell uname | cut -d _ -f 1), MINGW32)
		MK_DIR = mkdir -p
	endif

	ifeq ($(shell uname | cut -d _ -f 1), MINGW64)
		MK_DIR = mkdir -p
	endif

	ifeq ($(shell uname | cut -d _ -f 1), DARWIN)
		MK_DIR = mkdir -p
	endif
endif

# List the subdirectories for creating object files
SUB_DIRS +=  \
hpl/adc \
usb/class/cdc/device \
hpl/ramecc \
samd51a/gcc/gcc \
hpl/evsys \
hpl/qspi \
examples \
hpl/systick \
hpl/oscctrl \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0 \
stdio_redirect \
hpl/tc \
hpl/osc32kctrl \
usb \
hpl/dmac \
hal/src \
stdio_redirect/gcc \
samd51a/gcc \
hal/utils/src \
hpl/wdt \
hpl/rtc \
hpl/trng \
usb/class/hid/device \
hpl/usb \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/GCC/ARM_CM4F \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/MemMang \
hpl/mclk \
hpl/eic \
hpl/cmcc \
hpl/gclk \
usb/device \
hpl/pm \
usb/class/msc/device \
hpl/pac \
hpl/sercom \
hpl/core \
hpl/nvmctrl \
hpl/dsu

# List the object files
OBJS +=  \
hal/src/hal_io.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/list.o \
hpl/pac/hpl_pac.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/GCC/ARM_CM4F/port.o \
hal/src/hal_i2c_m_async.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/event_groups.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/MemMang/heap_4.o \
hpl/wdt/hpl_wdt.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/croutine.o \
usb/usb_protocol.o \
stdio_redirect/stdio_io.o \
hal/src/hal_spi_m_dma.o \
stdio_redirect/gcc/write.o \
samd51a/gcc/system_samd51.o \
usb/class/hid/device/hiddf_generic.o \
hal/src/hal_init.o \
hpl/evsys/hpl_evsys.o \
hpl/core/hpl_core_m4.o \
hal/src/hal_cache.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/stream_buffer.o \
usb/class/cdc/device/cdcdf_acm.o \
hal/utils/src/utils_syscalls.o \
hpl/qspi/hpl_qspi.o \
hpl/nvmctrl/hpl_nvmctrl.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/queue.o \
usb_start.o \
hal/src/hal_timer.o \
hpl/usb/hpl_usb.o \
stdio_redirect/gcc/read.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/timers.o \
hal/src/hal_delay.o \
hpl/pm/hpl_pm.o \
hpl/core/hpl_init.o \
hpl/ramecc/hpl_ramecc.o \
hal/src/hal_wdt.o \
usb/class/hid/device/hiddf_mouse.o \
hal/utils/src/utils_assert.o \
hpl/dmac/hpl_dmac.o \
hal/src/hal_adc_async.o \
hpl/oscctrl/hpl_oscctrl.o \
hal/src/hal_rand_sync.o \
hpl/trng/hpl_trng.o \
usb/class/hid/device/hiddf_keyboard.o \
hal/src/hal_usart_sync.o \
hpl/mclk/hpl_mclk.o \
hpl/gclk/hpl_gclk.o \
stdio_start.o \
hal/src/hal_flash.o \
hal/src/hal_spi_m_async.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/rtos_port.o \
hal/src/hal_crc_sync.o \
hal/src/hal_usb_device.o \
rtos_start.o \
hal/utils/src/utils_list.o \
hpl/osc32kctrl/hpl_osc32kctrl.o \
hpl/rtc/hpl_rtc.o \
examples/driver_examples.o \
driver_init.o \
hal/src/hal_usart_async.o \
usb/class/msc/device/mscdf.o \
hpl/sercom/hpl_sercom.o \
hal/utils/src/utils_ringbuffer.o \
hal/src/hal_atomic.o \
main.o \
hal/src/hal_gpio.o \
hal/src/hal_ext_irq.o \
hpl/eic/hpl_eic.o \
hal/src/hal_qspi_dma.o \
thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/tasks.o \
hal/src/hal_evsys.o \
hal/utils/src/utils_event.o \
hal/src/hal_sleep.o \
hpl/adc/hpl_adc.o \
samd51a/gcc/gcc/startup_samd51.o \
atmel_start.o \
usb/device/usbdc.o \
hpl/tc/hpl_tc.o \
hpl/cmcc/hpl_cmcc.o \
hpl/systick/hpl_systick.o \
hpl/dsu/hpl_dsu.o

OBJS_AS_ARGS +=  \
"hal/src/hal_io.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/list.o" \
"hpl/pac/hpl_pac.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/GCC/ARM_CM4F/port.o" \
"hal/src/hal_i2c_m_async.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/event_groups.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/MemMang/heap_4.o" \
"hpl/wdt/hpl_wdt.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/croutine.o" \
"usb/usb_protocol.o" \
"stdio_redirect/stdio_io.o" \
"hal/src/hal_spi_m_dma.o" \
"stdio_redirect/gcc/write.o" \
"samd51a/gcc/system_samd51.o" \
"usb/class/hid/device/hiddf_generic.o" \
"hal/src/hal_init.o" \
"hpl/evsys/hpl_evsys.o" \
"hpl/core/hpl_core_m4.o" \
"hal/src/hal_cache.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/stream_buffer.o" \
"usb/class/cdc/device/cdcdf_acm.o" \
"hal/utils/src/utils_syscalls.o" \
"hpl/qspi/hpl_qspi.o" \
"hpl/nvmctrl/hpl_nvmctrl.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/queue.o" \
"usb_start.o" \
"hal/src/hal_timer.o" \
"hpl/usb/hpl_usb.o" \
"stdio_redirect/gcc/read.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/timers.o" \
"hal/src/hal_delay.o" \
"hpl/pm/hpl_pm.o" \
"hpl/core/hpl_init.o" \
"hpl/ramecc/hpl_ramecc.o" \
"hal/src/hal_wdt.o" \
"usb/class/hid/device/hiddf_mouse.o" \
"hal/utils/src/utils_assert.o" \
"hpl/dmac/hpl_dmac.o" \
"hal/src/hal_adc_async.o" \
"hpl/oscctrl/hpl_oscctrl.o" \
"hal/src/hal_rand_sync.o" \
"hpl/trng/hpl_trng.o" \
"usb/class/hid/device/hiddf_keyboard.o" \
"hal/src/hal_usart_sync.o" \
"hpl/mclk/hpl_mclk.o" \
"hpl/gclk/hpl_gclk.o" \
"stdio_start.o" \
"hal/src/hal_flash.o" \
"hal/src/hal_spi_m_async.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/rtos_port.o" \
"hal/src/hal_crc_sync.o" \
"hal/src/hal_usb_device.o" \
"rtos_start.o" \
"hal/utils/src/utils_list.o" \
"hpl/osc32kctrl/hpl_osc32kctrl.o" \
"hpl/rtc/hpl_rtc.o" \
"examples/driver_examples.o" \
"driver_init.o" \
"hal/src/hal_usart_async.o" \
"usb/class/msc/device/mscdf.o" \
"hpl/sercom/hpl_sercom.o" \
"hal/utils/src/utils_ringbuffer.o" \
"hal/src/hal_atomic.o" \
"main.o" \
"hal/src/hal_gpio.o" \
"hal/src/hal_ext_irq.o" \
"hpl/eic/hpl_eic.o" \
"hal/src/hal_qspi_dma.o" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/tasks.o" \
"hal/src/hal_evsys.o" \
"hal/utils/src/utils_event.o" \
"hal/src/hal_sleep.o" \
"hpl/adc/hpl_adc.o" \
"samd51a/gcc/gcc/startup_samd51.o" \
"atmel_start.o" \
"usb/device/usbdc.o" \
"hpl/tc/hpl_tc.o" \
"hpl/cmcc/hpl_cmcc.o" \
"hpl/systick/hpl_systick.o" \
"hpl/dsu/hpl_dsu.o"

# List the directories containing header files
DIR_INCLUDES +=  \
-I"../" \
-I"../config" \
-I"../examples" \
-I"../hal/include" \
-I"../hal/utils/include" \
-I"../hpl/adc" \
-I"../hpl/cmcc" \
-I"../hpl/core" \
-I"../hpl/dmac" \
-I"../hpl/eic" \
-I"../hpl/dsu" \
-I"../hpl/evsys" \
-I"../hpl/gclk" \
-I"../hpl/mclk" \
-I"../hpl/nvmctrl" \
-I"../hpl/osc32kctrl" \
-I"../hpl/oscctrl" \
-I"../hpl/pac" \
-I"../hpl/pm" \
-I"../hpl/port" \
-I"../hpl/qspi" \
-I"../hpl/ramecc" \
-I"../hpl/rtc" \
-I"../hpl/sercom" \
-I"../hpl/systick" \
-I"../hpl/tc" \
-I"../hpl/trng" \
-I"../hpl/usb" \
-I"../hpl/wdt" \
-I"../hri" \
-I"../" \
-I"../config" \
-I"../thirdparty/RTOS" \
-I"../thirdparty/RTOS/freertos/FreeRTOSV10.0.0" \
-I"../thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/include" \
-I"../thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/GCC/ARM_CM4F" \
-I"../thirdparty/RTOS/freertos/FreeRTOSV10.0.0/module_config" \
-I"../" \
-I"../config" \
-I"../usb" \
-I"../usb/class/cdc" \
-I"../usb/class/cdc/device" \
-I"../usb/class/composite/device" \
-I"../usb/class/hid" \
-I"../usb/class/hid/device" \
-I"../usb/class/hub" \
-I"../usb/class/msc" \
-I"../usb/class/msc/device" \
-I"../usb/class/vendor" \
-I"../usb/device" \
-I"../" \
-I"../config" \
-I"../stdio_redirect" \
-I"../" \
-I"../CMSIS/Core/Include" \
-I"../samd51a/include"

D51_INC_DIR += $(DIR_INCLUDES)

# List the dependency files
DEPS := $(OBJS:%.o=%.d)

DEPS_AS_ARGS +=  \
"usb/usb_protocol.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/event_groups.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/croutine.d" \
"hpl/wdt/hpl_wdt.d" \
"stdio_redirect/stdio_io.d" \
"hal/utils/src/utils_event.d" \
"hal/src/hal_io.d" \
"hpl/ramecc/hpl_ramecc.d" \
"hpl/qspi/hpl_qspi.d" \
"hpl/core/hpl_core_m4.d" \
"hpl/evsys/hpl_evsys.d" \
"hal/utils/src/utils_syscalls.d" \
"usb/class/cdc/device/cdcdf_acm.d" \
"samd51a/gcc/system_samd51.d" \
"stdio_redirect/gcc/write.d" \
"hal/src/hal_spi_m_dma.d" \
"hpl/usb/hpl_usb.d" \
"main.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/timers.d" \
"stdio_redirect/gcc/read.d" \
"usb/class/msc/device/mscdf.d" \
"hpl/nvmctrl/hpl_nvmctrl.d" \
"hal/src/hal_init.d" \
"hal/src/hal_timer.d" \
"usb/class/hid/device/hiddf_mouse.d" \
"hal/src/hal_wdt.d" \
"hpl/cmcc/hpl_cmcc.d" \
"hal/src/hal_adc_async.d" \
"hpl/dmac/hpl_dmac.d" \
"hal/utils/src/utils_assert.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/stream_buffer.d" \
"hal/src/hal_delay.d" \
"hal/src/hal_crc_sync.d" \
"hpl/core/hpl_init.d" \
"hpl/pm/hpl_pm.d" \
"hal/src/hal_flash.d" \
"stdio_start.d" \
"hpl/gclk/hpl_gclk.d" \
"hal/src/hal_usb_device.d" \
"hpl/pac/hpl_pac.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/rtos_port.d" \
"hal/src/hal_spi_m_async.d" \
"hal/src/hal_usart_sync.d" \
"usb/class/hid/device/hiddf_keyboard.d" \
"hpl/trng/hpl_trng.d" \
"hal/src/hal_rand_sync.d" \
"hpl/mclk/hpl_mclk.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/GCC/ARM_CM4F/port.d" \
"driver_init.d" \
"usb_start.d" \
"hpl/systick/hpl_systick.d" \
"hal/src/hal_usart_async.d" \
"hpl/osc32kctrl/hpl_osc32kctrl.d" \
"rtos_start.d" \
"examples/driver_examples.d" \
"hpl/rtc/hpl_rtc.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/portable/MemMang/heap_4.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/list.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/queue.d" \
"hal/src/hal_qspi_dma.d" \
"hal/src/hal_cache.d" \
"hal/utils/src/utils_list.d" \
"hal/src/hal_sleep.d" \
"hal/utils/src/utils_ringbuffer.d" \
"hpl/sercom/hpl_sercom.d" \
"hal/src/hal_evsys.d" \
"hal/src/hal_gpio.d" \
"hal/src/hal_ext_irq.d" \
"hpl/eic/hpl_eic.d" \
"usb/class/hid/device/hiddf_generic.d" \
"hal/src/hal_i2c_m_async.d" \
"hal/src/hal_atomic.d" \
"hpl/dsu/hpl_dsu.d" \
"hpl/tc/hpl_tc.d" \
"usb/device/usbdc.d" \
"hpl/oscctrl/hpl_oscctrl.d" \
"hpl/adc/hpl_adc.d" \
"thirdparty/RTOS/freertos/FreeRTOSV10.0.0/Source/tasks.d" \
"samd51a/gcc/gcc/startup_samd51.d" \
"atmel_start.d"

OUTPUT_FILE_NAME :=AtmelStart
QUOTE := "
OUTPUT_FILE_PATH +=$(OUTPUT_FILE_NAME).elf
OUTPUT_FILE_PATH_AS_ARGS +=$(OUTPUT_FILE_NAME).elf

vpath %.c ../
vpath %.s ../
vpath %.S ../

# All Target
all: $(SUB_DIRS) $(OUTPUT_FILE_PATH)

# Linker target

$(OUTPUT_FILE_PATH): $(COM_OBJS) $(D51_OBJS) $(OBJS) | $(COM_OBJ_DIR) $(D51_OBJ_DIR)
	@echo Building target: $@
	@echo Invoking: ARM/GNU Linker
	$(QUOTE)arm-none-eabi-gcc$(QUOTE) -o $(OUTPUT_FILE_NAME).elf $(OBJS_AS_ARGS) -Wl,--start-group -lm -Wl,--end-group -mthumb \
-Wl,-Map="$(OUTPUT_FILE_NAME).map" --specs=nosys.specs -Wl,--gc-sections -Wl,--print-memory-usage -mcpu=cortex-m4 -u _printf_float\
 \
-T"../grid/samd51n20a_flash_bootloader.ld" \
-L"../samd51a/gcc/gcc" \

	@echo Finished building target: $@

	"arm-none-eabi-objcopy" -O binary "$(OUTPUT_FILE_NAME).elf" "$(OUTPUT_FILE_NAME).bin"
	"arm-none-eabi-objcopy" -O ihex -R .eeprom -R .fuse -R .lock -R .signature  \
        "$(OUTPUT_FILE_NAME).elf" "$(OUTPUT_FILE_NAME).hex"
	"arm-none-eabi-objcopy" -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma \
        .eeprom=0 --no-change-warnings -O binary "$(OUTPUT_FILE_NAME).elf" \
        "$(OUTPUT_FILE_NAME).eep" || exit 0
	"arm-none-eabi-objdump" -h -S "$(OUTPUT_FILE_NAME).elf" > "$(OUTPUT_FILE_NAME).lss"
	"arm-none-eabi-size" "$(OUTPUT_FILE_NAME).elf"



# Compiler targets

%.o: %.c
	@echo Building file: $<
	@echo ARM/GNU C Compiler
	$(QUOTE)arm-none-eabi-gcc$(QUOTE) -x c -mthumb $(GRID_DEFINES) -DDEBUG -Os -ffunction-sections -mlong-calls -g3 -Wall -c -std=gnu99 \
-D__SAMD51N20A__ -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 \
$(DIR_INCLUDES) $(COM_INC_DIR) $(D51_INC_DIR) \
-MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -o "$@" "$<"
	@echo Finished building: $<

%.o: %.s
	@echo Building file: $<
	@echo ARM/GNU Assembler
	$(QUOTE)arm-none-eabi-as$(QUOTE) -x c -mthumb $(GRID_DEFINES) -DDEBUG -Os -ffunction-sections -mlong-calls -g3 -Wall -c -std=gnu99 \
-D__SAMD51N20A__ -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 \
$(DIR_INCLUDES) $(COM_INC_DIR) $(D51_INC_DIR) \
-MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -o "$@" "$<"
	@echo Finished building: $<

%.o: %.S
	@echo Building file: $<
	@echo ARM/GNU Preprocessing Assembler
	$(QUOTE)arm-none-eabi-gcc$(QUOTE) -x c -mthumb $(GRID_DEFINES) -DDEBUG -Os -ffunction-sections -mlong-calls -g3 -Wall -c -std=gnu99 \
-D__SAMD51N20A__ -mcpu=cortex-m4 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 \
$(DIR_INCLUDES) $(COM_INC_DIR) $(D51_INC_DIR) \
-MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"  -o "$@" "$<"
	@echo Finished building: $<

# Detect changes in the dependent files and recompile the respective object files.
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif
endif


$(SUB_DIRS):
	$(MK_DIR) "$@"

clean:
	rm -f $(OBJS_AS_ARGS)
	rm -f $(OUTPUT_FILE_PATH)
	rm -f $(DEPS_AS_ARGS)
	rm -f $(OUTPUT_FILE_NAME).a $(OUTPUT_FILE_NAME).hex $(OUTPUT_FILE_NAME).bin \
        $(OUTPUT_FILE_NAME).lss $(OUTPUT_FILE_NAME).eep $(OUTPUT_FILE_NAME).map \
        $(OUTPUT_FILE_NAME).srec

# ================== DEBUG WITH AUTOGDB =================
debug: GRID_DEFINES = -DGRID_BUILD_DEBUG
debug: $(SUB_DIRS) $(OUTPUT_FILE_PATH) debug_post
debug_post:
	arm-none-eabi-gdb -iex "target extended-remote localhost:3333" $(OUTPUT_FILE_NAME).elf

# ================== RELEASE FOR AUTOMATION =================
release: clean $(SUB_DIRS) $(OUTPUT_FILE_PATH) release_post
release: GRID_DEFINES = -DGRID_BUILD_RELEASE
release_post:
	python3 ../../tools/uf2conv/uf2conv.py -c -b 0x4000 -o ./grid_fw.uf2 ./$(OUTPUT_FILE_NAME).bin
