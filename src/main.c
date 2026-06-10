#include "core/buffer.h"
#include "core/config.h"
#include "core/draw.h"
#include "core/flags.h"
#include "core/hud.h"
#include "util/log.h"
#include "wayland/listeners/global.h"
#include <linux/limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;

static void handle_signal(int signum) {
    (void)signum;
    running = 0;
}

static int section_changed(const struct fmt_section *a, const struct fmt_section *b) {
    if (strcmp(a->text, b->text) != 0 || a->nspans != b->nspans)
        return 1;
    for (size_t i = 0; i < a->nspans; i++) {
        const struct fmt_span *x = &a->spans[i], *y = &b->spans[i];
        if (x->start != y->start || x->len != y->len || x->has_color != y->has_color ||
            (x->has_color && x->color != y->color))
            return 1;
    }
    return 0;
}

static int hud_info_changed(const struct hud_info *a, const struct hud_info *b) {
    return section_changed(&a->left, &b->left) || section_changed(&a->center, &b->center) ||
           section_changed(&a->right, &b->right);
}

int main(int argc, char **argv) {
    // Layout style: separated pills by default, full-width background with --full.
    enum hud_style style = HUD_STYLE_SEPARATED;

    int flags = parse_flags(argc, argv);
    if (flags & FLAG_HELP) {
        printf(
            "Usage: baguette [options]\n"
            "\n"
            "Options:\n"
            "  -h, --help        Show this help message\n"
            "  -f, --full        Use full HUD style\n"
            "  -s, --separated   Use separated HUD style\n"
            "\n"
        );
        return 0;
    }

    if (flags & FLAG_FULL)
        style = HUD_STYLE_FULL;
    else if (flags & FLAG_SEPARATED)
        style = HUD_STYLE_SEPARATED;

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

    if (read_config(&state) < 0) {
        ERROR("cannot read hud config.");
        return -1;
    }

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

    struct hud_info last = {0};
    int drawn = 0;

    while (running) {
        wl_display_roundtrip(display);
        int want_scale = state.scale > 0 ? state.scale : 1;
        int rebuilt = 0;
        if (want_scale != state.rendered_scale) {
            INFO("rescaling buffer: %d -> %d", state.rendered_scale, want_scale);
            if (init_buffer(&state) < 0)
                ERROR("failed to rebuild buffer on rescale.");
            else
                rebuilt = 1; // new buffer has no pixels yet -> must repaint
        }

        hud_info_process(state.info, &stdin_fd);

        if (!drawn || rebuilt || hud_info_changed(&last, state.info)) {
            draw_hud(&state, state.info);

            wl_surface_attach(state.surface, state.buffer, 0, 0);
            wl_surface_damage_buffer(state.surface, 0, 0, state.width, state.height);
            wl_surface_commit(state.surface);
            wl_display_flush(display);

            last = *state.info;
            drawn = 1;
        }
    }

    closelog();
    if (hud_state_destroy(&state) < 0)
        ERROR("failed to destory hud state");

    return 0;
}
