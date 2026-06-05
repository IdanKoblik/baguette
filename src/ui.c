#define _GNU_SOURCE

#include "ui.h"

#include <unistd.h>
#include <sys/mman.h>
#include "log.h"

int init_surface(struct hud_state *state) {
    if (!state)
        return -1;

    INFO("creating new surface.");
    struct wl_surface *surface = wl_compositor_create_surface(state->compositor);
    if (!surface) {
        ERROR("failed to create surface.");
        return  -1;
    }
    state->surface = surface;

    INFO("creating new layer surface.");
    struct zwlr_layer_surface_v1 *layer_surface = zwlr_layer_shell_v1_get_layer_surface(state->layer_shell, surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_TOP, NAMESPACE);
    if (!layer_surface) {
        ERROR("failed to create layer surface.");
        return  -1;
    }

    state->layer_surface = layer_surface;
    zwlr_layer_surface_v1_set_anchor(layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);

    zwlr_layer_surface_v1_set_size(layer_surface, 0, HEIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, HEIGHT);

    wl_surface_commit(surface);
    return 0;
}

int init_buffer(struct hud_state *state) {
    if (!state)
        return -1;

    INFO("creating new layer buffer.");
    int fd = memfd_create("", 0);
    if (fd < 0) {
        ERROR("failed to init annon file.");
        return -1;
    }

    int size = (state->height * state->width) * sizeof(uint32_t);
    if (ftruncate(fd, size) < 0) {
        ERROR("failed to truncate annon file.");
        return -1;
    }

    uint32_t *pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pixels == MAP_FAILED) {
        ERROR("failed to map shm buffer.");
        return -1;
    }
    state->pixels = pixels;

    struct wl_shm_pool *shm_pool = wl_shm_create_pool(state->shm, fd, size);
    if (!shm_pool) {
        ERROR("failed to init shm pool.");
        return -1;
    }
    state->shm_pool = shm_pool;

    int stride = state->width * sizeof(uint32_t);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(shm_pool, 0, state->width, state->height, stride, WL_SHM_FORMAT_ARGB8888);
    if (!buffer) {
        ERROR("failed to create buffer.");
        return -1;
    }
    state->buffer = buffer;

    // Fill the whole buffer with opaque red (ARGB8888: 0xAARRGGBB).
    for (uint32_t i = 0; i < state->width * state->height; i++)
        pixels[i] = 0xFFFF0000;

    wl_surface_attach(state->surface, buffer, 0, 0);
    wl_surface_damage_buffer(state->surface, 0, 0, state->width, state->height);
    wl_surface_commit(state->surface);

    return 0;
}
