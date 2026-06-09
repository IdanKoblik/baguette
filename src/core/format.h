#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Wire format for stdin frames.
//
// One line is one frame. A tab splits the line into up to three sections:
//
//     LEFT  \t  CENTER  \t  RIGHT
//
// The third section keeps any further tabs literally. Each section is plain
// text with an optional colour:
//
//     %{#rrggbb}   set this section's foreground colour
//     %{-}         reset to the default colour
//     %%           a literal '%'
//
// A section has a single colour; if several colour tags appear, the last one
// wins. Anything that is not a recognised tag (unknown body, missing '}') is
// kept literally, so no input is ever silently dropped.

#define FMT_REGION_DELIM '\t'
#define FMT_MAX_TEXT     256 // bytes per section, including the NUL

// One section: its text and a single colour. has_color == false means "use the
// default colour" (no tag, or a %{-} reset).
struct fmt_section {
    char     text[FMT_MAX_TEXT];
    uint32_t color;     // 0xRRGGBB, meaningful only when has_color is true
    bool     has_color;
};

struct fmt_frame {
    struct fmt_section left;
    struct fmt_section center;
    struct fmt_section right;
};

int fmt_decode(const char *input, struct fmt_frame *out);

int fmt_encode(const struct fmt_frame *frame, char *buf, size_t cap);
