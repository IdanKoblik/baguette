#include "greatest.h"
#include "core/draw.h"
#include "core/hud.h"
#include <cairo/cairo.h>
#include <stdint.h>

// Build a hud_state backed by an in-memory Cairo surface so draw_hud() can run
// with no Wayland compositor. Caller must free with free_test_state().
static void make_test_state(struct hud_state *state, int w, int h) {
    *state = (struct hud_state){0};
    state->width = w;
    state->height = h;
    state->cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    state->cairo = cairo_create(state->cairo_surface);
}

static void free_test_state(struct hud_state *state) {
    cairo_destroy(state->cairo);
    cairo_surface_destroy(state->cairo_surface);
}

static uint32_t pixel_at(struct hud_state *state, int x, int y) {
    cairo_surface_flush(state->cairo_surface);
    const unsigned char *data = cairo_image_surface_get_data(state->cairo_surface);
    int stride = cairo_image_surface_get_stride(state->cairo_surface);
    return *(const uint32_t *)(data + y * stride + x * 4);
}

TEST draw_null_state_is_safe(void) {
    draw_hud(NULL, "a", "b", "c");
    PASS();
}

TEST draw_paints_background(void) {
    struct hud_state state;
    make_test_state(&state, 80, 40);

    draw_hud(&state, NULL, NULL, NULL);

    uint32_t corner = pixel_at(&state, 0, 0);
    uint32_t center = pixel_at(&state, 40, 20);

    // ARGB32 is 0xAARRGGBB, premultiplied. Background is opaque grey ~0x10.
    ASSERT_EQ_FMT((uint32_t)0xFF, corner >> 24, "%x");   // alpha
    ASSERT_EQ_FMT(corner, center, "%x");                 // uniform fill

    uint8_t r = (corner >> 16) & 0xFF;
    uint8_t g = (corner >> 8) & 0xFF;
    uint8_t b = corner & 0xFF;
    ASSERT_EQ(r, g);
    ASSERT_EQ(g, b);
    ASSERT(r <= 20); // ~16, allow rounding slack
    free_test_state(&state);
    PASS();
}

TEST draw_empty_strings_behave_like_null(void) {
    struct hud_state state;
    make_test_state(&state, 80, 40);

    draw_hud(&state, "", "", "");

    uint32_t corner = pixel_at(&state, 0, 0);
    uint8_t r = (corner >> 16) & 0xFF;
    ASSERT(r <= 20);
    free_test_state(&state);
    PASS();
}

TEST draw_with_text_is_valid(void) {
    struct hud_state state;
    make_test_state(&state, 200, 40);

    draw_hud(&state, "left", "center", "right");

    ASSERT_EQ(CAIRO_STATUS_SUCCESS, cairo_status(state.cairo));
    free_test_state(&state);
    PASS();
}

SUITE(draw_suite) {
    RUN_TEST(draw_null_state_is_safe);
    RUN_TEST(draw_paints_background);
    RUN_TEST(draw_empty_strings_behave_like_null);
    RUN_TEST(draw_with_text_is_valid);
}
