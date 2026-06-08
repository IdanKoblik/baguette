#define _GNU_SOURCE

#include <unistd.h>
#include <sys/mman.h>
#include "buffer.h"
#include "../util/log.h"

int init_buffer(struct hud_state *state) {
    if (!state) {
        ERROR("cannot init buffer, hud state is null.");
        return -1;
    }

    INFO("creating new layer buffer.");
    int fd = memfd_create("", 0);
    if (fd < 0) {
        ERROR("failed to init annon file.");
        return -1;
    }

    // The surface is sized in logical pixels, but on a scaled output we must
    // allocate the buffer at device resolution and tell the compositor the
    // scale. Otherwise a 1x buffer gets upscaled and the text looks blurry.
    int scale = state->scale > 0 ? state->scale : 1;
    int dev_width = state->width * scale;
    int dev_height = state->height * scale;

    int stride = dev_width * sizeof(uint32_t);
    int size = dev_height * stride;
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

    struct wl_buffer *buffer = wl_shm_pool_create_buffer(shm_pool, 0, dev_width, dev_height, stride, WL_SHM_FORMAT_ARGB8888);
    if (!buffer) {
        ERROR("failed to create buffer.");
        return -1;
    }
    state->buffer = buffer;

    // Wrap the shm pixels in a Cairo surface so we can draw straight into the
    // buffer. CAIRO_FORMAT_ARGB32 is native-endian premultiplied 0xAARRGGBB,
    // which matches WL_SHM_FORMAT_ARGB8888.
    state->cairo_surface = cairo_image_surface_create_for_data( (unsigned char *)pixels, CAIRO_FORMAT_ARGB32, dev_width, dev_height, stride);
    if (cairo_surface_status(state->cairo_surface) != CAIRO_STATUS_SUCCESS) {
        ERROR("failed to create cairo surface.");
        return -1;
    }

    state->cairo = cairo_create(state->cairo_surface);
    if (cairo_status(state->cairo) != CAIRO_STATUS_SUCCESS) {
        ERROR("failed to create cairo context.");
        return -1;
    }

    // Draw using logical coordinates; Cairo maps them onto device pixels.
    cairo_scale(state->cairo, scale, scale);

    wl_surface_set_buffer_scale(state->surface, scale);
    wl_surface_attach(state->surface, buffer, 0, 0);
    wl_surface_damage_buffer(state->surface, 0, 0, dev_width, dev_height);
    wl_surface_commit(state->surface);

    return 0;
}
