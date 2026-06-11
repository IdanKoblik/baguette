#pragma once

#include <wayland-client.h>

extern struct wl_registry_listener registry_listener;

void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface,
                            uint32_t version);
