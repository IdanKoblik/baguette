CC      := cc
CFLAGS  := -Wall -Wextra -std=c17 -D_XOPEN_SOURCE=700 -Isrc $(shell pkg-config --cflags wayland-client cairo) $(EXTRA_CFLAGS)
LDLIBS  := $(shell pkg-config --libs wayland-client cairo) -lm -lconfig

SRC_DIR := src
OBJ_DIR := build
BIN     := baguette
LOG     := baguette.log
MAN1    := man/baguette.1

# Install layout. Override on the command line, e.g.
#   make PREFIX=/usr DESTDIR="$pkgdir" install
PREFIX     := /usr/local
BINDIR     := $(PREFIX)/bin
MANDIR     := $(PREFIX)/share/man
MAN1DIR    := $(MANDIR)/man1
LICENSEDIR := $(PREFIX)/share/licenses/$(BIN)
INSTALL    := install

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

PROTOCOLS := $(wildcard protocols/*.xml)

GENERATED_HEADERS := $(PROTOCOLS:protocols/%.xml=src/wayland/protocols/%-protocol.h)
GENERATED_SOURCES := $(PROTOCOLS:protocols/%.xml=src/wayland/protocols/%-protocol.c)

TEST_DIR  := tests
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_BIN  := $(OBJ_DIR)/test_runner
# Link every production object except main.o (which owns its own main()).
TEST_OBJS := $(filter-out $(OBJ_DIR)/main.o,$(OBJS))

# Coverage: instrument the production sources (except main.c).
COV_DIR  := coverage
COV_OBJ  := $(COV_DIR)/obj
COV_SRCS := $(filter-out $(SRC_DIR)/main.c,$(SRCS)) $(TEST_SRCS)
COV_OBJS := $(COV_SRCS:%.c=$(COV_OBJ)/%.o)

CLANG_FORMAT := clang-format
# Format only hand-written sources: skip generated protocol code and the
# vendored greatest.h test header.
FORMAT_SRCS  := $(shell find $(SRC_DIR) $(TEST_DIR) \( -name '*.c' -o -name '*.h' \) 2>/dev/null | grep -vE '/protocols/|tests/greatest\.h')

.PHONY: all clean run compdb protocol test coverage logs install uninstall format format-check

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

# Export this app's journald logs to a greppable file
logs:
	journalctl -t $(BIN) -o short-iso > $(LOG)
	@echo "Logs written to $(LOG)"

test: protocol $(TEST_OBJS) $(TEST_SRCS)
	$(CC) $(CFLAGS) -I$(TEST_DIR) $(TEST_SRCS) $(TEST_OBJS) -o $(TEST_BIN) $(LDLIBS)
	./$(TEST_BIN)

# Requires lcov
coverage: protocol $(COV_OBJS)
	$(CC) $(COV_OBJS) --coverage -o $(COV_DIR)/test_runner $(LDLIBS)
	./$(COV_DIR)/test_runner
	lcov --capture --directory $(COV_OBJ) --output-file $(COV_DIR)/coverage.info
	lcov --remove $(COV_DIR)/coverage.info '*/tests/*' '/usr/*' --output-file $(COV_DIR)/coverage.info
	genhtml $(COV_DIR)/coverage.info --output-directory $(COV_DIR)/html
	@echo "Coverage report: $(COV_DIR)/html/index.html"

$(COV_OBJ)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(TEST_DIR) --coverage -c $< -o $@

# Format all hand-written sources in place (requires clang-format).
format:
	$(CLANG_FORMAT) -i $(FORMAT_SRCS)

# Check formatting without modifying files; fails if any file needs formatting.
format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_SRCS)

# Install the binary, man page and license into $(DESTDIR)$(PREFIX).
install: $(BIN)
	$(INSTALL) -Dm755 $(BIN) $(DESTDIR)$(BINDIR)/$(BIN)
	$(INSTALL) -Dm644 $(MAN1) $(DESTDIR)$(MAN1DIR)/$(BIN).1
	$(INSTALL) -Dm644 LICENSE $(DESTDIR)$(LICENSEDIR)/LICENSE

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)
	rm -f $(DESTDIR)$(MAN1DIR)/$(BIN).1
	rm -f $(DESTDIR)$(LICENSEDIR)/LICENSE

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(LOG) $(COV_DIR) $(GENERATED_SOURCES) $(GENERATED_HEADERS)

-include $(DEPS)
