#pragma once

#include "../protocols/wlr-layer-shell-unstable-v1-protocol.h"

extern struct zwlr_layer_surface_v1_listener layer_surface_listener;

void layer_surface_handle(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1,
                          uint32_t serial, uint32_t width, uint32_t height);
