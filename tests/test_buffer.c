#include "greatest.h"
#include "core/buffer.h"

TEST geometry_unscaled(void) {
    struct buffer_geometry g = buffer_compute_geometry(100, 40, 1);
    ASSERT_EQ(1, g.scale);
    ASSERT_EQ(100, g.dev_width);
    ASSERT_EQ(40, g.dev_height);
    ASSERT_EQ(100 * 4, g.stride);
    ASSERT_EQ(40 * 100 * 4, g.size);
    PASS();
}

TEST geometry_scaled_2x(void) {
    struct buffer_geometry g = buffer_compute_geometry(100, 40, 2);
    ASSERT_EQ(2, g.scale);
    ASSERT_EQ(200, g.dev_width);
    ASSERT_EQ(80, g.dev_height);
    ASSERT_EQ(200 * 4, g.stride);
    ASSERT_EQ(80 * 200 * 4, g.size);
    PASS();
}

TEST geometry_zero_scale_clamps_to_one(void) {
    struct buffer_geometry g = buffer_compute_geometry(100, 40, 0);
    ASSERT_EQ(1, g.scale);
    ASSERT_EQ(100, g.dev_width);
    PASS();
}

TEST geometry_negative_scale_clamps_to_one(void) {
    struct buffer_geometry g = buffer_compute_geometry(100, 40, -3);
    ASSERT_EQ(1, g.scale);
    ASSERT_EQ(40, g.dev_height);
    PASS();
}

TEST geometry_stride_is_four_bytes_per_pixel(void) {
    struct buffer_geometry g = buffer_compute_geometry(37, 13, 3);
    ASSERT_EQ(g.dev_width * 4, g.stride);
    ASSERT_EQ(g.dev_height * g.stride, g.size);
    PASS();
}

SUITE(buffer_suite) {
    RUN_TEST(geometry_unscaled);
    RUN_TEST(geometry_scaled_2x);
    RUN_TEST(geometry_zero_scale_clamps_to_one);
    RUN_TEST(geometry_negative_scale_clamps_to_one);
    RUN_TEST(geometry_stride_is_four_bytes_per_pixel);
}
