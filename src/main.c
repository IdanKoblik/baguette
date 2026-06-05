#include <wayland-client.h>
#include "log.h"

void create_global_object(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
    // Ignore compiler warning
    (void)!data;
    (void)!wl_registry;

    LOG("%d, %s, %d", name, interface, version);
}

static struct wl_registry_listener registry_listener = {
    .global = create_global_object
};

int main(void) {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        ERROR("Could not connect to a Wayland display.");
        return 1;
    }

    LOG("Success: Connected to the Wayland display successfully!");
    struct wl_registry *registry = wl_display_get_registry(display);
    if (!registry) {
        ERROR("Failed to get display registry.");
        return 1;
    }

    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    wl_display_disconnect(display);
    return 0;
}
