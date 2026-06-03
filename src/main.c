#include <stdio.h>
#include <wayland-client.h>

int main(void) {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Error: Could not connect to a Wayland display.\n");
        return 1;
    }

    printf("Success: Connected to the Wayland display successfully!\n");
    wl_display_disconnect(display);
    return 0;
}
