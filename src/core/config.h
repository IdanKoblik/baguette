#pragma once

#include <stdint.h>

#define CONFIG_FILE "style.cfg"

struct hud_state;

struct color {
    double r;
    double g;
    double b;
};

struct config {
    const char *font;
    double font_size;

    double height;

    struct color background_color;
    double hud_padding;
    double radius;
    double vmargin;
    double pad_x;
};

int read_config(struct hud_state *state);
