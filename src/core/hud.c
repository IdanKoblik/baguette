#include "hud.h"
#include "../util/log.h"
#include "buffer.h"
#include "surface.h"
#include <sys/mman.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <stdlib.h>

int hud_state_init(struct hud_state *state, struct wl_registry *registry, struct wl_display *display) {
    if (!state) {
        ERROR("Cannot init hud state -> null.");
        return -1;
    }

    if (!registry) {
        ERROR("Cannot init hud_state, registry -> null.");
        return -1;
    }

    if (!display) {
        ERROR("Cannot init hud_state, display -> null.");
        return -1;
    }

    memset(state, 0, sizeof(*state));
    state->display = display;
    state->registry = registry;

    return 0;
}

int hud_state_active(struct hud_state *state) {
    if (!state) {
        ERROR("cannot activate hud state -> null.");
        return -1;
    }

    INFO("activating hud state.");
    if (init_surface(state) < 0) {
        ERROR("cannot init hud surface.");
        return -1;
    }

    wl_display_roundtrip(state->display);

    if (init_buffer(state) < 0) {
        ERROR("cannot init hud buffer.");
        return -1;
    }

    wl_display_roundtrip(state->display);
    struct hud_info *info = (struct hud_info *)malloc(sizeof(struct hud_info));
    if (!info) {
        ERROR("cannot allocate hud info.");
        return -1;
    }

    state->info = info;
    return 0;
}

void hud_info_process(struct hud_info *info, struct pollfd *stdin_fd) {
    if (!info) {
        ERROR("cannot update hud info -> null.");
        return;
    }

    if (!stdin_fd) {
        ERROR("cannot update hud info, stdin fd not found.");
        return;
    }

    const char *eof = "<EOF>";
    int ret = poll(stdin_fd, 1, 1000);
    if (ret < 0) {
        ERROR("something went wrong with poll.");
        return;
    }

    if (ret == 0)
        goto info_eof;

    if (!(stdin_fd->revents & POLLIN))
        return;

    char buf[MAX_INPUT];
    ssize_t n = read(stdin_fd->fd, buf, sizeof buf - 1);
    if (n > 0) {
        buf[n] = '\0';
        // Trim trailing newline(s)
        while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) { buf[--n] = '\0'; }

        snprintf(info->right,  MAX_INPUT, "%s", buf);
        snprintf(info->left,   MAX_INPUT, "%s", buf);
        snprintf(info->center, MAX_INPUT, "%s", buf);

        return;
    }

    if (n < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK))
        ERROR("cannot read from poll.");

info_eof:
    snprintf(info->right,  MAX_INPUT, "%s", eof);
    snprintf(info->left,   MAX_INPUT, "%s", eof);
    snprintf(info->center, MAX_INPUT, "%s", eof);
}

int hud_state_destroy(struct hud_state *state) {
    if (!state) {
        ERROR("Cannot destroy hud state -> null.");
        return -1;
    }

    INFO("Cleaning up hud state.");
    if (state->info)
        free(state->info);

    if (state->cairo)
        cairo_destroy(state->cairo);

    if (state->cairo_surface)
        cairo_surface_destroy(state->cairo_surface);

    if (state->pixels && state->map_size)
        munmap(state->pixels, state->map_size);

    if (state->layer_shell)
        zwlr_layer_shell_v1_destroy(state->layer_shell);

    if (state->surface)
        wl_surface_destroy(state->surface);

    if (state->layer_surface)
        zwlr_layer_surface_v1_destroy(state->layer_surface);

    if (state->shm)
        wl_shm_destroy(state->shm);

    if (state->shm_pool)
        wl_shm_pool_destroy(state->shm_pool);

    if (state->compositor)
        wl_compositor_destroy(state->compositor);

    if (state->registry)
        wl_registry_destroy(state->registry);

    if (state->display)
        wl_display_disconnect(state->display);

    return 0;
}
