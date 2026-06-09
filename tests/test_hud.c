#include "core/hud.h"
#include "greatest.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// hud_info_process() polls a fd and decodes the line it reads into the three
// tab-separated hud_info sections.
static int temp_fd_with(const char *contents) {
    char path[] = "/tmp/baguette_hud_XXXXXX";
    int fd = mkstemp(path);
    unlink(path); // unlinked but kept open; freed on close
    if (contents && *contents)
        (void)!write(fd, contents, strlen(contents));
    lseek(fd, 0, SEEK_SET);
    return fd;
}

TEST process_decodes_tab_separated_sections(void) {
    struct hud_info info = {0};
    int fd = temp_fd_with("L\tC\tR");
    struct pollfd pfd = {.fd = fd, .events = POLLIN};

    hud_info_process(&info, &pfd);
    close(fd);

    ASSERT_STR_EQ("L", info.left.text);
    ASSERT_STR_EQ("C", info.center.text);
    ASSERT_STR_EQ("R", info.right.text);
    PASS();
}

TEST process_trims_trailing_newline(void) {
    struct hud_info info = {0};
    int fd = temp_fd_with("world\n");
    struct pollfd pfd = {.fd = fd, .events = POLLIN};

    hud_info_process(&info, &pfd);
    close(fd);

    ASSERT_STR_EQ("world", info.left.text);
    PASS();
}

TEST process_trims_trailing_crlf(void) {
    struct hud_info info = {0};
    int fd = temp_fd_with("line\r\n");
    struct pollfd pfd = {.fd = fd, .events = POLLIN};

    hud_info_process(&info, &pfd);
    close(fd);

    ASSERT_STR_EQ("line", info.left.text); // no tabs -> all in the left section
    PASS();
}

TEST process_eof_yields_eof_marker(void) {
    struct hud_info info = {0};
    int fd = temp_fd_with(""); // empty file -> read() returns 0 (EOF)
    struct pollfd pfd = {.fd = fd, .events = POLLIN};

    hud_info_process(&info, &pfd);
    close(fd);

    ASSERT_STR_EQ("<EOF>", info.left.text);
    ASSERT_STR_EQ("<EOF>", info.center.text);
    ASSERT_STR_EQ("<EOF>", info.right.text);
    PASS();
}

TEST process_null_info_is_safe(void) {
    int fd = temp_fd_with("data");
    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    hud_info_process(NULL, &pfd); // must not crash
    close(fd);
    PASS();
}

TEST process_null_pollfd_is_safe(void) {
    struct hud_info info = {0};
    hud_info_process(&info, NULL); // must not crash, leaves info untouched
    ASSERT_STR_EQ("", info.left.text);
    PASS();
}

TEST init_rejects_null_args(void) {
    struct hud_state state;
    int dummy;
    struct wl_registry *reg = (struct wl_registry *)&dummy;
    struct wl_display *disp = (struct wl_display *)&dummy;

    ASSERT_EQ(-1, hud_state_init(NULL, reg, disp));
    ASSERT_EQ(-1, hud_state_init(&state, NULL, disp));
    ASSERT_EQ(-1, hud_state_init(&state, reg, NULL));
    PASS();
}

TEST init_zeroes_state_and_stores_handles(void) {
    struct hud_state state;
    int dummy;
    struct wl_registry *reg = (struct wl_registry *)&dummy;
    struct wl_display *disp = (struct wl_display *)&dummy;

    // Pre-dirty a field to confirm init memsets the struct.
    state.width = 0xdead;

    ASSERT_EQ(0, hud_state_init(&state, reg, disp));
    ASSERT_EQ((void *)disp, (void *)state.display);
    ASSERT_EQ((void *)reg, (void *)state.registry);
    ASSERT_EQ(0, state.width); // zeroed by memset
    ASSERT_EQ(NULL, (void *)state.info);
    PASS();
}

TEST destroy_rejects_null(void) {
    ASSERT_EQ(-1, hud_state_destroy(NULL));
    PASS();
}

TEST destroy_zeroed_state_is_noop(void) {
    // A zeroed state has all handles NULL, so destroy walks every guard
    // without touching Wayland/Cairo and returns success.
    struct hud_state state = {0};
    ASSERT_EQ(0, hud_state_destroy(&state));
    PASS();
}

SUITE(hud_suite) {
    RUN_TEST(process_decodes_tab_separated_sections);
    RUN_TEST(process_trims_trailing_newline);
    RUN_TEST(process_trims_trailing_crlf);
    RUN_TEST(process_eof_yields_eof_marker);
    RUN_TEST(process_null_info_is_safe);
    RUN_TEST(process_null_pollfd_is_safe);
    RUN_TEST(init_rejects_null_args);
    RUN_TEST(init_zeroes_state_and_stores_handles);
    RUN_TEST(destroy_rejects_null);
    RUN_TEST(destroy_zeroed_state_is_noop);
}
