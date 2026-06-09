#include "core/config.h"
#include "core/hud.h"
#include "greatest.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// read_config() reads $HOME/.config/baguette/style.cfg via libconfig. These
// tests stand up a throwaway $HOME, drop a style.cfg into it, and assert on the
// parsed struct config. HOME is saved and restored around each test so the
// suite leaves the environment untouched.

static char saved_home[4096];
static char tmp_home[] = "/tmp/baguette_cfg_XXXXXX";

// Point HOME at a fresh temp dir and create .config/baguette/ inside it.
static void set_temp_home(void) {
    const char *h = getenv("HOME");
    saved_home[0] = '\0';
    if (h)
        snprintf(saved_home, sizeof(saved_home), "%s", h);

    strcpy(tmp_home, "/tmp/baguette_cfg_XXXXXX");
    (void)!mkdtemp(tmp_home);
    setenv("HOME", tmp_home, 1);

    char dir[4096];
    snprintf(dir, sizeof(dir), "%s/.config", tmp_home);
    mkdir(dir, 0700);
    snprintf(dir, sizeof(dir), "%s/.config/baguette", tmp_home);
    mkdir(dir, 0700);
}

static void restore_home(void) {
    if (saved_home[0])
        setenv("HOME", saved_home, 1);
    else
        unsetenv("HOME");
}

// Write contents into $HOME/.config/baguette/style.cfg.
static void write_style_cfg(const char *contents) {
    char path[4096];
    snprintf(path, sizeof(path), "%s/.config/baguette/%s", tmp_home, CONFIG_FILE);
    FILE *f = fopen(path, "w");
    if (f) {
        fputs(contents, f);
        fclose(f);
    }
}

TEST config_rejects_null_state(void) {
    ASSERT_EQ(-1, read_config(NULL));
    PASS();
}

TEST config_fails_without_home(void) {
    const char *h = getenv("HOME");
    char prev[4096] = {0};
    if (h)
        snprintf(prev, sizeof(prev), "%s", h);
    unsetenv("HOME");

    struct hud_state state = {0};
    ASSERT_EQ(-1, read_config(&state));
    ASSERT_EQ(NULL, (void *)state.cfg);

    if (prev[0])
        setenv("HOME", prev, 1);
    PASS();
}

TEST config_fails_when_file_missing(void) {
    set_temp_home(); // dir exists but no style.cfg written

    struct hud_state state = {0};
    int rc = read_config(&state);

    restore_home();
    ASSERT_EQ(-1, rc);
    ASSERT_EQ(NULL, (void *)state.cfg);
    PASS();
}

TEST config_parses_all_fields(void) {
    set_temp_home();
    write_style_cfg("font = \"JetBrains Mono\";\n"
                    "font_size = 14.5;\n"
                    "background = \"#1e1e2e\";\n"
                    "hud_padding = 8;\n"
                    "radius = 12;\n"
                    "vmargin = 4;\n"
                    "pad_x = 6;\n");

    struct hud_state state = {0};
    int rc = read_config(&state);
    restore_home();

    ASSERT_EQ(0, rc);
    ASSERT(state.cfg != NULL);
    ASSERT_STR_EQ("JetBrains Mono", state.cfg->font);
    ASSERT_IN_RANGE(14.5, state.cfg->font_size, 0.0001);
    // "#1e1e2e" -> r=0x1e, g=0x1e, b=0x2e, each normalised to [0,1].
    ASSERT_IN_RANGE(0x1e / 255.0, state.cfg->background_color.r, 0.0001);
    ASSERT_IN_RANGE(0x1e / 255.0, state.cfg->background_color.g, 0.0001);
    ASSERT_IN_RANGE(0x2e / 255.0, state.cfg->background_color.b, 0.0001);
    ASSERT_EQ(8, state.cfg->hud_padding);
    ASSERT_EQ(12, state.cfg->radius);
    ASSERT_EQ(4, state.cfg->vmargin);
    ASSERT_EQ(6, state.cfg->pad_x);

    free(state.cfg);
    PASS();
}

TEST config_missing_keys_stay_zeroed(void) {
    set_temp_home();
    write_style_cfg("radius = 10;\n"); // only one key present

    struct hud_state state = {0};
    int rc = read_config(&state);
    restore_home();

    ASSERT_EQ(0, rc);
    ASSERT(state.cfg != NULL);
    ASSERT_EQ(10, state.cfg->radius);
    // Everything not present in the file is left as the memset(0) default.
    ASSERT_EQ(NULL, (void *)state.cfg->font);
    ASSERT_IN_RANGE(0.0, state.cfg->background_color.r, 0.0001);
    ASSERT_IN_RANGE(0.0, state.cfg->background_color.g, 0.0001);
    ASSERT_IN_RANGE(0.0, state.cfg->background_color.b, 0.0001);
    ASSERT_EQ(0, state.cfg->hud_padding);
    ASSERT_EQ(0, state.cfg->vmargin);
    ASSERT_EQ(0, state.cfg->pad_x);
    ASSERT_IN_RANGE(0.0, state.cfg->font_size, 0.0001);

    free(state.cfg);
    PASS();
}

TEST config_fails_on_malformed_file(void) {
    set_temp_home();
    write_style_cfg("this is = not valid ::: syntax\n");

    struct hud_state state = {0};
    int rc = read_config(&state);
    restore_home();

    ASSERT_EQ(-1, rc);
    ASSERT_EQ(NULL, (void *)state.cfg);
    PASS();
}

SUITE(config_suite) {
    RUN_TEST(config_rejects_null_state);
    RUN_TEST(config_fails_without_home);
    RUN_TEST(config_fails_when_file_missing);
    RUN_TEST(config_parses_all_fields);
    RUN_TEST(config_missing_keys_stay_zeroed);
    RUN_TEST(config_fails_on_malformed_file);
}
