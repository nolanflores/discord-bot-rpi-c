CC     := gcc
TARGET := run
MODE   ?= default

IMAGE  := discord_bot
DOCKER := docker

SRCS := src/main.c $(wildcard src/core/*.c) $(wildcard src/bot/*.c) $(wildcard src/discord/*.c)
PLUGIN_SRCS := $(wildcard plugins/*.c)

BUILD := build
BIN   := $(TARGET)
OBJS  := $(SRCS:%.c=$(BUILD)/%.o)
DEPS  := $(OBJS:.o=.d)
PLUGIN_BINS := $(PLUGIN_SRCS:plugins/%.c=$(BUILD)/plugins/%.so)

CFLAGS  := -std=gnu11 -Wall -Wextra -I include -I . -DPLUGIN_DIRECTORY='"$(BUILD)/plugins"'
LDLIBS  := -lssl -lcrypto -ldl

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

all: $(BIN) plugins

debug:
	$(MAKE) MODE=debug all

pi:
	$(MAKE) MODE=pi all

release:
	$(DOCKER) build -t $(IMAGE) .

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ -rdynamic $(LDLIBS)

plugins: $(PLUGIN_BINS)

$(BUILD)/plugins/%.so: plugins/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@

# -MMD -MP generates .d files so header changes trigger rebuilds
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(DEPS)