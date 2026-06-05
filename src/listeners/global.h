#pragma once

#include <wayland-client.h>

void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

