# Do not use the built-in inference rules
.SUFFIXES:

CC = cc

BUILD_DIR = build
BUILD_DIRS = \
	$(BUILD_DIR) \
	$(BUILD_DIR)/common \
	$(BUILD_DIR)/common/src \
	$(BUILD_DIR)/common/src/c \
	$(BUILD_DIR)/common/test \
	$(BUILD_DIR)/common/dep \
	$(BUILD_DIR)/common/dep/lua-5.5.0 \
	$(BUILD_DIR)/common/dep/lua-5.5.0/src \

$(BUILD_DIRS): ; mkdir $@

SRC_DIR = .
SRCS = \
	test.c \
	platform.c \
	\
	common/src/c/grid_math.c \
	common/src/c/grid_noflash.c \
	common/src/c/grid_swsr.c \
	common/src/c/grid_ui.c \
	common/src/c/grid_ui_encoder.c \
	common/test/grid_ui_encoder_test.c \
	common/test/grid_lua_precision_test.c \
	\
	common/dep/lua-5.5.0/src/lapi.c \
	common/dep/lua-5.5.0/src/lauxlib.c \
	common/dep/lua-5.5.0/src/lbaselib.c \
	common/dep/lua-5.5.0/src/lcode.c \
	common/dep/lua-5.5.0/src/lcorolib.c \
	common/dep/lua-5.5.0/src/lctype.c \
	common/dep/lua-5.5.0/src/ldblib.c \
	common/dep/lua-5.5.0/src/ldebug.c \
	common/dep/lua-5.5.0/src/ldo.c \
	common/dep/lua-5.5.0/src/ldump.c \
	common/dep/lua-5.5.0/src/lfunc.c \
	common/dep/lua-5.5.0/src/lgc.c \
	common/dep/lua-5.5.0/src/linit.c \
	common/dep/lua-5.5.0/src/liolib.c \
	common/dep/lua-5.5.0/src/llex.c \
	common/dep/lua-5.5.0/src/lmathlib.c \
	common/dep/lua-5.5.0/src/lmem.c \
	common/dep/lua-5.5.0/src/loadlib.c \
	common/dep/lua-5.5.0/src/lobject.c \
	common/dep/lua-5.5.0/src/lopcodes.c \
	common/dep/lua-5.5.0/src/loslib.c \
	common/dep/lua-5.5.0/src/lparser.c \
	common/dep/lua-5.5.0/src/lstate.c \
	common/dep/lua-5.5.0/src/lstring.c \
	common/dep/lua-5.5.0/src/lstrlib.c \
	common/dep/lua-5.5.0/src/ltable.c \
	common/dep/lua-5.5.0/src/ltablib.c \
	common/dep/lua-5.5.0/src/ltm.c \
	common/dep/lua-5.5.0/src/lundump.c \
	common/dep/lua-5.5.0/src/lutf8lib.c \
	common/dep/lua-5.5.0/src/lvm.c \
	common/dep/lua-5.5.0/src/lzio.c \

INCLUDES = \
	-I. \
	-I.. \
	-Icommon/build/lua \
	-Icommon/dep \
	-Icommon/dep/lua-5.5.0/src \
	-Icommon/dep/proto \
	-Icommon/src/c \

DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c | $(BUILD_DIRS)
	$(CC) $(INCLUDES) -MM -MF $@ -MT $@ $<

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/%.d | $(BUILD_DIRS)
	$(CC) -g $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/test: $(OBJS) $(DEPS) | $(BUILD_DIRS)
	$(CC) -Wl,--unresolved-symbols=ignore-in-object-files -no-pie $(OBJS) -o $@ -lm

.PHONY: all
all: $(BUILD_DIR)/test

include $(DEPS)
