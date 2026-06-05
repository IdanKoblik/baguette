
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <sys/mman.h>
#include "hud.h"
#include "log.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

void hud_state_destroy(struct hud_state *state) {
    if (!state)
        return;

    INFO("cleaning up hud_state.");
    if (state->pixels)
        munmap(state->pixels, (state->width * state->height) * sizeof(uint32_t));

    if (state->compositor)
        wl_compositor_destroy(state->compositor);

    if (state->layer_shell)
        zwlr_layer_shell_v1_destroy(state->layer_shell);

    if (state->shm)
        wl_shm_destroy(state->shm);

    if (state->surface)
        wl_surface_destroy(state->surface);

    if (state->layer_surface)
        zwlr_layer_surface_v1_destroy(state->layer_surface);

    if (state->shm)
        wl_shm_destroy(state->shm);

    if (state->shm_pool)
        wl_shm_pool_destroy(state->shm_pool);
}
