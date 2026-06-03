#include <wayland-client.h>
#include "log.h"

int main(void) {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        ERROR("Error: Could not connect to a Wayland display.");
        return 1;
    }

    LOG("Success: Connected to the Wayland display successfully!");

    wl_display_disconnect(display);
    return 0;
}
