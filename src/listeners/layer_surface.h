#pragma once

#include "../wlr-layer-shell-unstable-v1-protocol.h"

void layer_surface_handle(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial, uint32_t width, uint32_t height);
