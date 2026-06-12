#include "config.h"
#include "../util/log.h"
#include "hud.h"
#include <errno.h>
#include <libconfig.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CFG_LOOKUP_STRING(cfg, key, dst) config_lookup_string((cfg), (key), (const char **)&(dst))
#define CFG_LOOKUP_FLOAT(cfg, key, dst) config_lookup_float((cfg), (key), &(dst))

static struct config *config_create_default(void) {
    struct config *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;

    cfg->font_size = 14.0f;
    cfg->height = 32.0f;
    cfg->hud_padding = 8.0f;
    cfg->radius = 5.0f;
    cfg->vmargin = 4.0f;
    cfg->pad_x = 8.0f;

    cfg->font = strdup("monospace");

    cfg->background_color.r = 0.0;
    cfg->background_color.g = 0.0;
    cfg->background_color.b = 0.0;

    return cfg;
}

static int write_default_config(const char *path, struct config *def_config) {
    if (!def_config) {
        ERROR("missing default config.");
        return -1;
    }

    char *dir = strrchr(path, '/') ? strndup(path, strrchr(path, '/') - path) : NULL;
    if (dir) {
        int made = mkdir(dir, 0755);
        free(dir);
        if (made != 0 && errno != EEXIST) {
            ERROR("cannot create config directory.");
            return -1;
        }
    }

    config_t cfg;
    config_init(&cfg);

    config_setting_t *root = config_root_setting(&cfg);
    config_setting_t *s;

    s = config_setting_add(root, "font_size", CONFIG_TYPE_FLOAT);
    config_setting_set_float(s, def_config->font_size);

    s = config_setting_add(root, "font", CONFIG_TYPE_STRING);
    config_setting_set_string(s, def_config->font);

    s = config_setting_add(root, "height", CONFIG_TYPE_FLOAT);
    config_setting_set_float(s, def_config->height);

    s = config_setting_add(root, "background", CONFIG_TYPE_STRING);
    config_setting_set_string(s, "#000000");

    s = config_setting_add(root, "hud_padding", CONFIG_TYPE_FLOAT);
    config_setting_set_float(s, def_config->hud_padding);

    s = config_setting_add(root, "radius", CONFIG_TYPE_FLOAT);
    config_setting_set_float(s, def_config->radius);

    s = config_setting_add(root, "vmargin", CONFIG_TYPE_FLOAT);
    config_setting_set_float(s, def_config->vmargin);

    s = config_setting_add(root, "pad_x", CONFIG_TYPE_FLOAT);
    config_setting_set_float(s, def_config->pad_x);

    int ok = config_write_file(&cfg, path);

    config_destroy(&cfg);

    return ok ? 0 : -1;
}

int read_config(struct hud_state *state) {
    if (!state) {
        ERROR("cannot read hud config -> null.");
        return -1;
    }

    const char *home_path = getenv("HOME");
    if (!home_path) {
        ERROR("cannot find home path.");
        return -1;
    }

    char cfg_path[PATH_MAX];
    int len = snprintf(cfg_path, sizeof(cfg_path), "%s/.config/baguette/%s", home_path, CONFIG_FILE);
    if (len < 0 || (size_t)len >= sizeof(cfg_path)) {
        ERROR("cannot encode config path.");
        return -1;
    }

    if (access(cfg_path, F_OK) != 0) {
        struct config *def_cfg = config_create_default();
        if (!def_cfg) {
            ERROR("cannot init default config.");
            return -1;
        }

        return write_default_config(cfg_path, def_cfg);
    }

    config_t config;
    config_init(&config);

    // Without this, config_lookup_float() fails (and leaves the field at 0) for
    // any setting written as a whole number, e.g. `radius = 5;` — only `5.0`
    // would load. Auto-convert lets int and float be used interchangeably.
    config_set_auto_convert(&config, CONFIG_TRUE);

    if (!config_read_file(&config, cfg_path)) {
        ERROR("failed to read from hud config file.");
        config_destroy(&config);

        return -1;
    }

    struct config *cfg = (struct config *)malloc(sizeof(struct config));
    if (!cfg) {
        ERROR("failed to create config");
        return -1;
    }

    memset(cfg, 0, sizeof(*cfg));

    // libconfig owns the strings returned by config_lookup_string(); they are
    // freed by config_destroy() below. strdup() so the config struct keeps its
    // own copies and the pointers don't dangle once the config_t is destroyed.
    const char *font = NULL, *background = NULL;

    CFG_LOOKUP_FLOAT(&config, "font_size", cfg->font_size);
    CFG_LOOKUP_STRING(&config, "font", font);

    CFG_LOOKUP_FLOAT(&config, "height", cfg->height);

    CFG_LOOKUP_STRING(&config, "background", background);
    CFG_LOOKUP_FLOAT(&config, "hud_padding", cfg->hud_padding);
    CFG_LOOKUP_FLOAT(&config, "radius", cfg->radius);
    CFG_LOOKUP_FLOAT(&config, "vmargin", cfg->vmargin);
    CFG_LOOKUP_FLOAT(&config, "pad_x", cfg->pad_x);

    cfg->font = font ? strdup(font) : NULL;
    if (background) {
        unsigned int r, g, b;

        /* Accept "#RRGGBB" */
        if (sscanf(background, "#%2x%2x%2x", &r, &g, &b) != 3)
            return false;

        cfg->background_color.r = r / 255.0;
        cfg->background_color.g = g / 255.0;
        cfg->background_color.b = b / 255.0;
    }

    config_destroy(&config);
    state->cfg = cfg;

    return 0;
}
