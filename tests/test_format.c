#include "greatest.h"
#include "core/format.h"
#include <string.h>

TEST decode_rejects_null_args(void) {
    struct fmt_frame f;
    ASSERT_EQ(-1, fmt_decode(NULL, &f));
    ASSERT_EQ(-1, fmt_decode("x", NULL));
    PASS();
}

TEST decode_empty_input_yields_empty_sections(void) {
    struct fmt_frame f;
    ASSERT_EQ(0, fmt_decode("", &f));
    ASSERT_STR_EQ("", f.left.text);
    ASSERT_STR_EQ("", f.center.text);
    ASSERT_STR_EQ("", f.right.text);
    ASSERT_EQ(false, f.left.has_color);
    PASS();
}

TEST decode_plain_text_goes_to_left_uncolored(void) {
    struct fmt_frame f;
    fmt_decode("hello", &f);

    ASSERT_STR_EQ("hello", f.left.text);
    ASSERT_EQ(false, f.left.has_color);
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
    ASSERT_EQ(true, f.left.has_color);
    ASSERT_EQ_FMT(0xa6e3a1u, f.left.color, "%x");

    ASSERT_STR_EQ("12:34", f.center.text);
    ASSERT_EQ(false, f.center.has_color); // no tag -> default colour

    ASSERT_STR_EQ("BAT", f.right.text);
    ASSERT_EQ(false, f.right.has_color);
    PASS();
}

TEST decode_color_tag_anywhere_in_section(void) {
    struct fmt_frame f;
    fmt_decode("CPU %{#ff0000}42%", &f); // tag mid-text

    ASSERT_STR_EQ("CPU 42%", f.left.text);
    ASSERT_EQ(true, f.left.has_color);
    ASSERT_EQ_FMT(0xff0000u, f.left.color, "%x");
    PASS();
}

TEST decode_uppercase_hex_is_accepted(void) {
    struct fmt_frame f;
    fmt_decode("%{#FF00AB}x", &f);
    ASSERT_EQ_FMT(0xff00abu, f.left.color, "%x");
    PASS();
}

TEST decode_last_color_tag_wins(void) {
    struct fmt_frame f;
    fmt_decode("%{#ff0000}a%{#00ff00}b", &f);

    ASSERT_STR_EQ("ab", f.left.text);
    ASSERT_EQ_FMT(0x00ff00u, f.left.color, "%x"); // last tag wins
    PASS();
}

TEST decode_reset_clears_color(void) {
    struct fmt_frame f;
    fmt_decode("%{#ff0000}hi%{-}there", &f);

    ASSERT_STR_EQ("hithere", f.left.text);
    ASSERT_EQ(false, f.left.has_color);
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
    ASSERT_EQ(false, f.left.has_color);
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
    ASSERT_EQ(false, f.left.has_color);
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
    RUN_TEST(decode_last_color_tag_wins);
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
