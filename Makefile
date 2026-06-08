CC      := cc
CFLAGS  := -Wall -Wextra -pedantic -std=c17 -D_POSIX_C_SOURCE=200809L -Isrc $(shell pkg-config --cflags wayland-client cairo)
LDLIBS  := $(shell pkg-config --libs wayland-client cairo)

SRC_DIR := src
OBJ_DIR := build
BIN     := baguette

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

PROTOCOLS := $(wildcard protocols/*.xml)

GENERATED_HEADERS := $(PROTOCOLS:protocols/%.xml=src/wayland/protocols/%-protocol.h)
GENERATED_SOURCES := $(PROTOCOLS:protocols/%.xml=src/wayland/protocols/%-protocol.c)

.PHONY: all clean run compdb protocol

all: $(BIN)

# Requires wayland-scanner package
protocol: $(GENERATED_HEADERS) $(GENERATED_SOURCES)
src/wayland/protocols/%-protocol.h: protocols/%.xml
	wayland-scanner client-header $< $@

src/wayland/protocols/%-protocol.c: protocols/%.xml
	wayland-scanner private-code $< $@

# Generate compile_commands.json for clangd (requires `bear`)
compdb:
	bear -- $(MAKE) -B all

$(BIN): protocol $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(GENERATED_SOURCES) $(GENERATED_HEADERS)

-include $(DEPS)
