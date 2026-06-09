#include "greatest.h"

SUITE_EXTERN(buffer_suite);
SUITE_EXTERN(draw_suite);
SUITE_EXTERN(log_suite);
SUITE_EXTERN(hud_suite);
SUITE_EXTERN(format_suite);

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(buffer_suite);
    RUN_SUITE(draw_suite);
    RUN_SUITE(log_suite);
    RUN_SUITE(hud_suite);
    RUN_SUITE(format_suite);
    GREATEST_MAIN_END();
}
