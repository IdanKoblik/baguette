#include "greatest.h"
#include "core/format.h"
#include <string.h>

TEST decode_rejects_null_args(void) {
    struct fmt_frame f;
    ASSERT_EQ(-1, fmt_decode(NULL, &f));
    ASSERT_EQ(-1, fmt_decode("x", NULL));
    PASS();
}

// Convenience: the colour in effect at the start of a section.
static bool sec_has_color(const struct fmt_section *s) { return s->spans[0].has_color; }
static uint32_t sec_color(const struct fmt_section *s) { return s->spans[0].color; }

TEST decode_empty_input_yields_empty_sections(void) {
    struct fmt_frame f;
    ASSERT_EQ(0, fmt_decode("", &f));
    ASSERT_STR_EQ("", f.left.text);
    ASSERT_STR_EQ("", f.center.text);
    ASSERT_STR_EQ("", f.right.text);
    ASSERT_EQ(false, sec_has_color(&f.left));
    PASS();
}

TEST decode_plain_text_goes_to_left_uncolored(void) {
    struct fmt_frame f;
    fmt_decode("hello", &f);

    ASSERT_STR_EQ("hello", f.left.text);
    ASSERT_EQ(false, sec_has_color(&f.left));
    ASSERT_EQ(1, (int)f.left.nspans); // plain text -> one default-coloured span
    ASSERT_STR_EQ("", f.center.text);
    ASSERT_STR_EQ("", f.right.text);
    PASS();
}

TEST decode_splits_three_sections_on_tabs(void) {
    struct fmt_frame f;
    fmt_decode("L\tC\tR", &f);

    ASSERT_STR_EQ("L", f.left.text);
    ASSERT_STR_EQ("C", f.center.text);
    ASSERT_STR_EQ("R", f.right.text);
    PASS();
}

TEST decode_third_section_keeps_extra_tabs_literal(void) {
    struct fmt_frame f;
    fmt_decode("a\tb\tc\td", &f);

    ASSERT_STR_EQ("a", f.left.text);
    ASSERT_STR_EQ("b", f.center.text);
    ASSERT_STR_EQ("c\td", f.right.text);
    PASS();
}

TEST decode_missing_sections_are_empty(void) {
    struct fmt_frame f;
    fmt_decode("only\t", &f); // left + empty center, no right

    ASSERT_STR_EQ("only", f.left.text);
    ASSERT_STR_EQ("", f.center.text);
    ASSERT_STR_EQ("", f.right.text);
    PASS();
}

TEST decode_color_tag_sets_section_color(void) {
    struct fmt_frame f;
    fmt_decode("%{#a6e3a1}CPU\t12:34\tBAT", &f);

    ASSERT_STR_EQ("CPU", f.left.text);
    ASSERT_EQ(true, sec_has_color(&f.left));
    ASSERT_EQ_FMT(0xa6e3a1u, sec_color(&f.left), "%x");

    ASSERT_STR_EQ("12:34", f.center.text);
    ASSERT_EQ(false, sec_has_color(&f.center)); // no tag -> default colour

    ASSERT_STR_EQ("BAT", f.right.text);
    ASSERT_EQ(false, sec_has_color(&f.right));
    PASS();
}

TEST decode_color_tag_anywhere_in_section(void) {
    struct fmt_frame f;
    fmt_decode("CPU %{#ff0000}42%", &f); // tag mid-text

    ASSERT_STR_EQ("CPU 42%", f.left.text);
    ASSERT_EQ(2, (int)f.left.nspans);

    // First run: "CPU " in the default colour.
    ASSERT_EQ(false, f.left.spans[0].has_color);
    ASSERT_EQ(0, (int)f.left.spans[0].start);
    ASSERT_EQ(4, (int)f.left.spans[0].len);

    // Second run: "42%" in red.
    ASSERT_EQ(true, f.left.spans[1].has_color);
    ASSERT_EQ_FMT(0xff0000u, f.left.spans[1].color, "%x");
    ASSERT_EQ(4, (int)f.left.spans[1].start);
    ASSERT_EQ(3, (int)f.left.spans[1].len);
    PASS();
}

TEST decode_uppercase_hex_is_accepted(void) {
    struct fmt_frame f;
    fmt_decode("%{#FF00AB}x", &f);
    ASSERT_EQ_FMT(0xff00abu, sec_color(&f.left), "%x");
    PASS();
}

// The bug from zaatar's right region: two coloured modules concatenated in one
// section must keep distinct colours instead of collapsing to the last one.
TEST decode_keeps_distinct_color_runs(void) {
    struct fmt_frame f;
    fmt_decode("%{#ea999c}vol%{#c6d0f5}up", &f);

    ASSERT_STR_EQ("volup", f.left.text);
    ASSERT_EQ(2, (int)f.left.nspans);

    ASSERT_EQ_FMT(0xea999cu, f.left.spans[0].color, "%x"); // red volume
    ASSERT_EQ(0, (int)f.left.spans[0].start);
    ASSERT_EQ(3, (int)f.left.spans[0].len);

    ASSERT_EQ_FMT(0xc6d0f5u, f.left.spans[1].color, "%x"); // blue uptime
    ASSERT_EQ(3, (int)f.left.spans[1].start);
    ASSERT_EQ(2, (int)f.left.spans[1].len);
    PASS();
}

TEST decode_reset_clears_color(void) {
    struct fmt_frame f;
    fmt_decode("%{#ff0000}hi%{-}there", &f);

    ASSERT_STR_EQ("hithere", f.left.text);
    ASSERT_EQ(2, (int)f.left.nspans);
    ASSERT_EQ(true, f.left.spans[0].has_color);  // "hi" red
    ASSERT_EQ(false, f.left.spans[1].has_color); // "there" default
    ASSERT_EQ(2, (int)f.left.spans[1].start);
    ASSERT_EQ(5, (int)f.left.spans[1].len);
    PASS();
}

TEST decode_escaped_percent_is_literal(void) {
    struct fmt_frame f;
    fmt_decode("50%% off", &f);
    ASSERT_STR_EQ("50% off", f.left.text);
    PASS();
}

TEST decode_trailing_percent_is_literal(void) {
    struct fmt_frame f;
    fmt_decode("50%", &f);
    ASSERT_STR_EQ("50%", f.left.text);
    PASS();
}

TEST decode_unknown_tag_is_kept_literal(void) {
    struct fmt_frame f;
    fmt_decode("%{x}y", &f);
    ASSERT_STR_EQ("%{x}y", f.left.text);
    ASSERT_EQ(false, sec_has_color(&f.left));
    PASS();
}

TEST decode_unterminated_tag_is_kept_literal(void) {
    struct fmt_frame f;
    fmt_decode("%{#fff", &f);
    ASSERT_STR_EQ("%{#fff", f.left.text);
    PASS();
}

TEST decode_bad_hex_length_is_kept_literal(void) {
    struct fmt_frame f;
    fmt_decode("%{#12345}z", &f); // only 5 hex digits
    ASSERT_EQ(false, sec_has_color(&f.left));
    ASSERT_STR_EQ("%{#12345}z", f.left.text);
    PASS();
}

TEST encode_rejects_null_args(void) {
    struct fmt_frame f = {0};
    char buf[8];
    ASSERT_EQ(-1, fmt_encode(NULL, buf, sizeof buf));
    ASSERT_EQ(-1, fmt_encode(&f, NULL, sizeof buf));
    ASSERT_EQ(-1, fmt_encode(&f, buf, 0));
    PASS();
}

TEST encode_empty_frame_is_two_tabs(void) {
    struct fmt_frame f = {0};
    char buf[16];
    int n = fmt_encode(&f, buf, sizeof buf);
    ASSERT_EQ(2, n);
    ASSERT_STR_EQ("\t\t", buf);
    PASS();
}

TEST encode_emits_color_tag_and_escapes_percent(void) {
    struct fmt_frame f;
    fmt_decode("%{#ff0000}50%% off\t\t", &f);

    char buf[64];
    fmt_encode(&f, buf, sizeof buf);
    ASSERT_STR_EQ("%{#ff0000}50%% off\t\t", buf);
    PASS();
}

TEST encode_roundtrips_three_colored_sections(void) {
    const char *in = "%{#ff0000}CPU\t%{#00ff00}12:34\tBAT";
    struct fmt_frame f;
    fmt_decode(in, &f);

    char buf[64];
    fmt_encode(&f, buf, sizeof buf);
    ASSERT_STR_EQ(in, buf);
    PASS();
}

TEST encode_roundtrips_multiple_color_runs(void) {
    // Distinct runs and a reset must all survive the decode/encode round-trip.
    const char *in = "%{#ea999c}vol%{#c6d0f5}up%{-}plain\t\t";
    struct fmt_frame f;
    fmt_decode(in, &f);

    char buf[64];
    fmt_encode(&f, buf, sizeof buf);
    ASSERT_STR_EQ(in, buf);
    PASS();
}

TEST encode_truncates_to_cap_and_returns_full_length(void) {
    struct fmt_frame f;
    fmt_decode("abcdef\t\t", &f);

    char buf[4];
    int n = fmt_encode(&f, buf, sizeof buf); // needs "abcdef\t\t" = 8 bytes
    ASSERT_EQ(8, n);                          // full length reported
    ASSERT_EQ(3, (int)strlen(buf));           // but only cap-1 written
    ASSERT_STR_EQ("abc", buf);
    PASS();
}

SUITE(format_suite) {
    RUN_TEST(decode_rejects_null_args);
    RUN_TEST(decode_empty_input_yields_empty_sections);
    RUN_TEST(decode_plain_text_goes_to_left_uncolored);
    RUN_TEST(decode_splits_three_sections_on_tabs);
    RUN_TEST(decode_third_section_keeps_extra_tabs_literal);
    RUN_TEST(decode_missing_sections_are_empty);
    RUN_TEST(decode_color_tag_sets_section_color);
    RUN_TEST(decode_color_tag_anywhere_in_section);
    RUN_TEST(decode_uppercase_hex_is_accepted);
    RUN_TEST(decode_keeps_distinct_color_runs);
    RUN_TEST(decode_reset_clears_color);
    RUN_TEST(decode_escaped_percent_is_literal);
    RUN_TEST(decode_trailing_percent_is_literal);
    RUN_TEST(decode_unknown_tag_is_kept_literal);
    RUN_TEST(decode_unterminated_tag_is_kept_literal);
    RUN_TEST(decode_bad_hex_length_is_kept_literal);
    RUN_TEST(encode_rejects_null_args);
    RUN_TEST(encode_empty_frame_is_two_tabs);
    RUN_TEST(encode_emits_color_tag_and_escapes_percent);
    RUN_TEST(encode_roundtrips_three_colored_sections);
    RUN_TEST(encode_truncates_to_cap_and_returns_full_length);
}
