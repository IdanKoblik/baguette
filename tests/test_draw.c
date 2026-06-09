#include "core/draw.h"
#include "core/format.h"
#include "core/hud.h"
#include "greatest.h"
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

// Decode a wire line into a hud_info, the shape draw_hud() consumes.
static struct hud_info info_from(const char *line) {
    struct fmt_frame f;
    fmt_decode(line, &f);
    struct hud_info info = {.left = f.left, .center = f.center, .right = f.right};
    return info;
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
    draw_hud(NULL, NULL);
    PASS();
}

TEST draw_paints_background(void) {
    struct hud_state state;
    make_test_state(&state, 80, 40);
    state.style = HUD_STYLE_FULL; // full-width background to sample at the corner

    struct hud_info info = info_from("\t\t");
    draw_hud(&state, &info);

    // The background is a rounded rect inset from the bar edges, so sample
    // interior points rather than the (transparent) corner.
    uint32_t p1 = pixel_at(&state, 40, 20);
    uint32_t p2 = pixel_at(&state, 30, 15);

    // ARGB32 is 0xAARRGGBB, premultiplied. Background is opaque grey ~0x10.
    ASSERT_EQ_FMT((uint32_t)0xFF, p1 >> 24, "%x"); // alpha
    ASSERT_EQ_FMT(p1, p2, "%x");                   // uniform fill

    uint8_t r = (p1 >> 16) & 0xFF;
    uint8_t g = (p1 >> 8) & 0xFF;
    uint8_t b = p1 & 0xFF;
    ASSERT_EQ(r, g);
    ASSERT_EQ(g, b);
    ASSERT(r <= 20); // ~16, allow rounding slack
    free_test_state(&state);
    PASS();
}

TEST draw_empty_sections_behave_like_null(void) {
    struct hud_state state;
    make_test_state(&state, 80, 40);
    state.style = HUD_STYLE_FULL;

    struct hud_info info = info_from("\t\t");
    draw_hud(&state, &info);

    uint32_t corner = pixel_at(&state, 0, 0);
    uint8_t r = (corner >> 16) & 0xFF;
    ASSERT(r <= 20);
    free_test_state(&state);
    PASS();
}

TEST draw_with_text_is_valid(void) {
    struct hud_state state;
    make_test_state(&state, 200, 40);

    struct hud_info info = info_from("left\tcenter\tright");
    draw_hud(&state, &info);

    ASSERT_EQ(CAIRO_STATUS_SUCCESS, cairo_status(state.cairo));
    free_test_state(&state);
    PASS();
}

// A section with several colour runs must paint without error.
TEST draw_multicolor_section_is_valid(void) {
    struct hud_state state;
    make_test_state(&state, 300, 40);

    struct hud_info info = info_from("\t\t%{#ea999c}vol%{#c6d0f5}up%{-}plain");
    draw_hud(&state, &info);

    ASSERT_EQ(CAIRO_STATUS_SUCCESS, cairo_status(state.cairo));
    free_test_state(&state);
    PASS();
}

SUITE(draw_suite) {
    RUN_TEST(draw_null_state_is_safe);
    RUN_TEST(draw_paints_background);
    RUN_TEST(draw_empty_sections_behave_like_null);
    RUN_TEST(draw_with_text_is_valid);
    RUN_TEST(draw_multicolor_section_is_valid);
}
