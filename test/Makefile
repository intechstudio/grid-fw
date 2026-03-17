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

INCLUDES = \
	-I. \
	-I.. \
	-Icommon/build/lua \
	-Icommon/dep \
	-Icommon/dep/proto \
	-Icommon/src/c \

DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c | $(BUILD_DIRS)
	$(CC) $(INCLUDES) -MM -MF $@ -MT $@ $<

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/%.d | $(BUILD_DIRS)
	$(CC) -g $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/test: $(OBJS) $(DEPS) | $(BUILD_DIRS)
	$(CC) -Wl,--unresolved-symbols=ignore-in-object-files -no-pie $(OBJS) -o $@

.PHONY: all
all: $(BUILD_DIR)/test

#include $(DEPS)
