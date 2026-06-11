#include "layer_surface.h"
#include "../../core/hud.h"
#include "../../util/log.h"

struct zwlr_layer_surface_v1_listener layer_surface_listener = {.configure = layer_surface_handle};

void layer_surface_handle(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial,
                          uint32_t width, uint32_t height) {
    if (!data) {
        ERROR("provided data is null.");
        return;
    }

    struct hud_state *state = (struct hud_state *)data;
    if (state->height == 0)
        state->height = height;

    if (state->width == 0)
        state->width = width;

    INFO("Screen height: %d, width: %d", state->height, state->width);

    zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface_v1, serial);
}
