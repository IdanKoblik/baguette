CC      := cc
CFLAGS  := -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L -Isrc $(shell pkg-config --cflags wayland-client)
LDLIBS  := $(shell pkg-config --libs wayland-client)

SRC_DIR := src
OBJ_DIR := build
BIN     := baguette

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN)

-include $(DEPS)
