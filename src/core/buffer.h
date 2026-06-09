#pragma once

#include "hud.h"
#include <stdint.h>

struct buffer_geometry {
    int scale; // effective scale (>= 1)
    int dev_width;
    int dev_height;
    int stride; // bytes per row
    int size;
};

// helper: compute device-pixel buffer geometry. A scale <= 0 is treated as 1 (no scaling).
struct buffer_geometry buffer_compute_geometry(int width, int height, int32_t scale);

int init_buffer(struct hud_state *state);
