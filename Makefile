CC     := gcc
TARGET := run
MODE   ?= default

SRCS := src/main.c $(wildcard src/core/*.c) $(wildcard src/bot/*.c)

BUILD := build/$(MODE)
BIN   := $(BUILD)/$(TARGET)
OBJS  := $(SRCS:%.c=$(BUILD)/%.o)
DEPS  := $(OBJS:.o=.d)

CFLAGS  := -std=gnu11 -Wall -Wextra -I include -I .
LDLIBS  := -lssl -lcrypto

RELEASE_TOKEN := /run/secrets/discord_token
LOCAL_TOKEN   := ./test/discord_token.txt

# Mode-specific flags
ifeq ($(MODE),pi)
  CFLAGS += -O2 -march=native -mtune=cortex-a53 -DTOKEN_PATH='"$(LOCAL_TOKEN)"'
else ifeq ($(MODE),release)
  CFLAGS += -O2 -DTOKEN_PATH='"$(RELEASE_TOKEN)"'
else ifeq ($(MODE),debug)
  CFLAGS += -O0 -g -DTOKEN_PATH='"$(LOCAL_TOKEN)"'
else
  CFLAGS += -O0 -DTOKEN_PATH='"$(LOCAL_TOKEN)"'
endif

.PHONY: all debug pi release clean

all: $(BIN)
	cp $(BIN) $(TARGET)

debug:
	$(MAKE) MODE=debug all

pi:
	$(MAKE) MODE=pi all

release:
	$(MAKE) MODE=release all

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

# -MMD -MP generates .d files so header changes trigger rebuilds
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(DEPS)