#include "core/flags.h"
#include "greatest.h"

// parse_flags() scans argv[1..argc-1] and ORs together a bitmask for each
// recognized long option. argv[0] (the program name) is skipped, unknown
// arguments are ignored, and order/duplication don't matter.

TEST no_args_yields_zero(void) {
    char *argv[] = {"baguette"};
    ASSERT_EQ(0, parse_flags(1, argv));
    PASS();
}

TEST help_flag_set(void) {
    char *argv[] = {"baguette", "--help"};
    ASSERT_EQ(FLAG_HELP, parse_flags(2, argv));
    PASS();
}

TEST full_flag_set(void) {
    char *argv[] = {"baguette", "--full"};
    ASSERT_EQ(FLAG_FULL, parse_flags(2, argv));
    PASS();
}

TEST separated_flag_set(void) {
    char *argv[] = {"baguette", "--separated"};
    ASSERT_EQ(FLAG_SEPARATED, parse_flags(2, argv));
    PASS();
}

TEST short_help_flag_set(void) {
    char *argv[] = {"baguette", "-h"};
    ASSERT_EQ(FLAG_HELP, parse_flags(2, argv));
    PASS();
}

TEST short_full_flag_set(void) {
    char *argv[] = {"baguette", "-f"};
    ASSERT_EQ(FLAG_FULL, parse_flags(2, argv));
    PASS();
}

TEST short_separated_flag_set(void) {
    char *argv[] = {"baguette", "-s"};
    ASSERT_EQ(FLAG_SEPARATED, parse_flags(2, argv));
    PASS();
}

// Long and short spellings of the same flag fold into one bit.
TEST short_and_long_are_equivalent(void) {
    char *argv[] = {"baguette", "-f", "--full"};
    ASSERT_EQ(FLAG_FULL, parse_flags(3, argv));
    PASS();
}

TEST all_short_flags_combine(void) {
    char *argv[] = {"baguette", "-h", "-f", "-s"};
    ASSERT_EQ(FLAG_HELP | FLAG_FULL | FLAG_SEPARATED, parse_flags(4, argv));
    PASS();
}

TEST all_flags_combine(void) {
    char *argv[] = {"baguette", "--help", "--full", "--separated"};
    ASSERT_EQ(FLAG_HELP | FLAG_FULL | FLAG_SEPARATED, parse_flags(4, argv));
    PASS();
}

TEST flag_order_is_irrelevant(void) {
    char *argv[] = {"baguette", "--separated", "--help", "--full"};
    ASSERT_EQ(FLAG_HELP | FLAG_FULL | FLAG_SEPARATED, parse_flags(4, argv));
    PASS();
}

TEST duplicate_flags_are_idempotent(void) {
    char *argv[] = {"baguette", "--full", "--full", "--full"};
    ASSERT_EQ(FLAG_FULL, parse_flags(4, argv));
    PASS();
}

TEST unknown_flags_are_ignored(void) {
    char *argv[] = {"baguette", "--nope", "-x", "garbage", "--Help"};
    ASSERT_EQ(0, parse_flags(5, argv));
    PASS();
}

TEST unknown_flags_mixed_with_known(void) {
    char *argv[] = {"baguette", "--nope", "--full", "extra", "--help"};
    ASSERT_EQ(FLAG_HELP | FLAG_FULL, parse_flags(5, argv));
    PASS();
}

// argv[0] is never inspected, so a program named like a flag is still a no-op.
TEST argv0_is_skipped(void) {
    char *argv[] = {"--help"};
    ASSERT_EQ(0, parse_flags(1, argv));
    PASS();
}

// Flags present in argv past argc must not be read.
TEST argc_bounds_are_respected(void) {
    char *argv[] = {"baguette", "--full", "--help"};
    ASSERT_EQ(FLAG_FULL, parse_flags(2, argv));
    PASS();
}

SUITE(flags_suite) {
    RUN_TEST(no_args_yields_zero);
    RUN_TEST(help_flag_set);
    RUN_TEST(full_flag_set);
    RUN_TEST(separated_flag_set);
    RUN_TEST(short_help_flag_set);
    RUN_TEST(short_full_flag_set);
    RUN_TEST(short_separated_flag_set);
    RUN_TEST(short_and_long_are_equivalent);
    RUN_TEST(all_short_flags_combine);
    RUN_TEST(all_flags_combine);
    RUN_TEST(flag_order_is_irrelevant);
    RUN_TEST(duplicate_flags_are_idempotent);
    RUN_TEST(unknown_flags_are_ignored);
    RUN_TEST(unknown_flags_mixed_with_known);
    RUN_TEST(argv0_is_skipped);
    RUN_TEST(argc_bounds_are_respected);
}
