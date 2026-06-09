#include "core/hud.h"
#include "core/buffer.h"
#include "util/log.h"
#include "wayland/listeners/global.h"
#include "core/draw.h"
#include <linux/limits.h>
#include <signal.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>

static volatile sig_atomic_t running = 1;

static void handle_signal(int signum) {
    (void)signum;
    running = 0;
}

int main(int argc, char **argv) {
    // Layout style: separated pills by default, full-width background with --full.
    enum hud_style style = HUD_STYLE_SEPARATED;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--full") == 0)
            style = HUD_STYLE_FULL;
        else if (strcmp(argv[i], "--separated") == 0)
            style = HUD_STYLE_SEPARATED;
    }

    openlog(NULL, LOG_PID | LOG_PERROR, LOG_USER);

    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        ERROR("failed to init wayland display.");
        return -1;
    }

    INFO("Success: Connected to the Wayland display successfully!");
    struct wl_registry *registry = wl_display_get_registry(display);
    if (!registry) {
        ERROR("Failed to get display registry.");
        return -1;
    }

    INFO("Success: Connected to the display registry successfully!");
    struct hud_state state = {0};
    if (hud_state_init(&state, registry, display) < 0) {
        ERROR("failed to init hud state.");
        return -1;
    }
    state.style = style;

    wl_registry_add_listener(state.registry, &registry_listener, &state);
    wl_display_roundtrip(display); // Roundtrip 1: Gets globals (compositor, etc..)

    if (hud_state_active(&state) < 0) {
        ERROR("failed to activate hud.");
        return -1;
    }

    INFO("Success: Loaded hud successfully!");
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct pollfd stdin_fd;
    stdin_fd.fd = STDIN_FILENO;
    stdin_fd.events = POLLIN;

    while (running) {
        wl_display_roundtrip(display);
        int want_scale = state.scale > 0 ? state.scale : 1;
        if (want_scale != state.rendered_scale) {
            INFO("rescaling buffer: %d -> %d", state.rendered_scale, want_scale);
            if (init_buffer(&state) < 0)
                ERROR("failed to rebuild buffer on rescale.");
        }

        hud_info_process(state.info, &stdin_fd);
        draw_hud(&state, state.info->left, state.info->center, state.info->right);

        wl_surface_attach(state.surface, state.buffer, 0, 0);
        wl_surface_damage_buffer(state.surface, 0, 0, state.width, state.height);
        wl_surface_commit(state.surface);
        wl_display_flush(display);
    }

    closelog();
    if (hud_state_destroy(&state) < 0)
        ERROR("failed to destory hud state");

    return 0;
}
