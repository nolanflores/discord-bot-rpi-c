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

# Mode-specific flags
ifeq ($(MODE),pi)
  CFLAGS += -O2 -march=native -mtune=cortex-a53
else ifeq ($(MODE),debug)
  CFLAGS += -O0 -g
else
  CFLAGS += -O0
endif

.PHONY: all debug pi clean

all: $(BIN)
	cp $(BIN) $(TARGET)

debug:
	$(MAKE) MODE=debug all

pi:
	$(MAKE) MODE=pi all

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

# -MMD -MP generates .d files so header changes trigger rebuilds
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(DEPS)