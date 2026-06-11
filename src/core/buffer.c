#define _GNU_SOURCE

#include "buffer.h"
#include "../util/log.h"
#include <sys/mman.h>
#include <unistd.h>

struct buffer_geometry buffer_compute_geometry(int width, int height, int32_t scale) {
    int s = scale > 0 ? scale : 1;
    int dev_width = width * s;
    int dev_height = height * s;
    int stride = dev_width * (int)sizeof(uint32_t);
    return (struct buffer_geometry){
        .scale = s,
        .dev_width = dev_width,
        .dev_height = dev_height,
        .stride = stride,
        .size = dev_height * stride,
    };
}

// Free the resources tied to the current buffer so init_buffer can be called
// again (e.g. after the compositor reports a new scale). Safe on a fresh state.
static void buffer_release(struct hud_state *state) {
    if (state->cairo) {
        cairo_destroy(state->cairo);
        state->cairo = NULL;
    }
    if (state->cairo_surface) {
        cairo_surface_destroy(state->cairo_surface);
        state->cairo_surface = NULL;
    }
    if (state->buffer) {
        wl_buffer_destroy(state->buffer);
        state->buffer = NULL;
    }
    if (state->shm_pool) {
        wl_shm_pool_destroy(state->shm_pool);
        state->shm_pool = NULL;
    }
    if (state->pixels && state->map_size) {
        munmap(state->pixels, state->map_size);
        state->pixels = NULL;
        state->map_size = 0;
    }
}

int init_buffer(struct hud_state *state) {
    if (!state) {
        ERROR("cannot init buffer, hud state is null.");
        return -1;
    }

    buffer_release(state);

    INFO("creating new layer buffer.");
    int fd = memfd_create("", 0);
    if (fd < 0) {
        ERROR("failed to init annon file.");
        return -1;
    }

    // The surface is sized in logical pixels, but on a scaled output we must
    // allocate the buffer at device resolution and tell the compositor the
    // scale. Otherwise a 1x buffer gets upscaled and the text looks blurry.
    struct buffer_geometry geo = buffer_compute_geometry(state->width, state->height, state->scale);
    int scale = geo.scale;
    int dev_width = geo.dev_width;
    int dev_height = geo.dev_height;
    int stride = geo.stride;
    int size = geo.size;
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
    state->map_size = (size_t)size;

    struct wl_shm_pool *shm_pool = wl_shm_create_pool(state->shm, fd, size);
    if (!shm_pool) {
        ERROR("failed to init shm pool.");
        close(fd);
        return -1;
    }
    state->shm_pool = shm_pool;
    // The compositor keeps its own reference to the fd via the pool; ours is
    // no longer needed (matters because init_buffer can run again on rescale).
    close(fd);

    struct wl_buffer *buffer =
        wl_shm_pool_create_buffer(shm_pool, 0, dev_width, dev_height, stride, WL_SHM_FORMAT_ARGB8888);
    if (!buffer) {
        ERROR("failed to create buffer.");
        return -1;
    }
    state->buffer = buffer;

    // Wrap the shm pixels in a Cairo surface so we can draw straight into the
    // buffer. CAIRO_FORMAT_ARGB32 is native-endian premultiplied 0xAARRGGBB,
    // which matches WL_SHM_FORMAT_ARGB8888.
    state->cairo_surface = cairo_image_surface_create_for_data((unsigned char *)pixels, CAIRO_FORMAT_ARGB32, dev_width,
                                                               dev_height, stride);
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
    state->rendered_scale = scale;

    wl_surface_set_buffer_scale(state->surface, scale);
    wl_surface_attach(state->surface, buffer, 0, 0);
    wl_surface_damage_buffer(state->surface, 0, 0, dev_width, dev_height);
    wl_surface_commit(state->surface);

    return 0;
}
